#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
    #define FD_SETSIZE    10000
    #define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
    #define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数
    #include <windows.h>
    #include <WinSock2.h>
    //静态链接库，解析WSAStartup和WSACleanup
    #pragma comment(lib,"ws2_32.lib")
#else
    #include <unistd.h> //unix std，类似windows.h
    #include <arpa/inet.h> //类似WinSock2.h
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif //_WIN32

#include <stdio.h>
#include "MessageHeader.hpp"

//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
    #define RECV_BUFF_SIZE 10240
#endif

class EasyTcpClient
{
private:
    SOCKET _sock;
    bool _isConnect;
    //第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE];
    //消息缓冲区的数据尾部位置
    int _lastPos;

public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
        _isConnect = false;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
    }

    virtual ~EasyTcpClient()
    {
        Close();
    }

    //初始化Socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        //启动Windows socket 2.x环境
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        if (isRun())
        {
            printf("<socket=%d>关闭旧连接...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("<socket=%d>错误，建立Socket失败...\n", (int)_sock);
        }
        else
        {
            //printf("建立<socket=%d>成功...\n", (int)_sock);
        }
        return _sock;
    }

    //连接服务器
    int Connect(const char* ip, unsigned short port)
    {
        if (INVALID_SOCKET == _sock)
        {
            InitSocket();
        }
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port); //host to net unsigned short
#ifdef _WIN32
        _sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
        _sin.sin_addr.s_addr = inet_addr(ip);
#endif
        int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>错误，连接服务器<%s, %d>失败...\n", (int)_sock, ip, port);
        }
        else
        {
            _isConnect = true;
            //printf("<socket=%d>连接服务器<%s, %d>成功...\n", (int)_sock, ip, port);
        }
        return ret;
    }

    //发送数据
    int SendData(DataHeader* header)
    {
        int ret = SOCKET_ERROR;
        if (isRun() && header)
        {
            ret = (int)send(_sock, (const char*)header, header->dataLength, 0);
            if (SOCKET_ERROR == ret)
            {
                Close();
            }
        }
        return ret;
    }

    //处理网络消息
    bool OnRun()
    {
        if (isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock, &fdReads);
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_sock + 1, &fdReads, 0, 0, &t);
            if (ret < 0) //select错误
            {
                printf("<socket=%d>select任务结束1\n", (int)_sock);
                Close();
                return false;
            }
            if (FD_ISSET(_sock, &fdReads))
            {
                FD_CLR(_sock, &fdReads);
                if (-1 == RecvData(_sock)) //与服务器断开连接
                {
                    printf("<socket=%d>select任务结束2\n", (int)_sock);
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    //接收数据 处理粘包 拆分包
    int RecvData(SOCKET cSock)
    {
        //接收服务端数据
        char* szRecv = _szMsgBuf + _lastPos;
        int nlen = (int)recv(cSock, szRecv, RECV_BUFF_SIZE - _lastPos, 0); //返回其实际copy的字节数
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            printf("<socket=%d>与服务器断开连接，任务结束。\n", (int)cSock);
            return -1;
        }
        //将收取收取到的数据拷贝到消息缓冲区
        //memcpy(_szMsgBuf + _lastPos, _szRecv, nlen);
        //消息缓冲区的数据尾部位置后移
        _lastPos += nlen;
        //判断消息缓冲区的数据长度大于消息头DataHeader长度
        while (_lastPos >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息的长度
            DataHeader* header = (DataHeader*)_szMsgBuf;
            //判断消息缓冲区的数据长度大于消息长度
            if (_lastPos >= header->dataLength)
            {
                //剩余未处理消息缓冲区消息的长度
                int nSize = _lastPos - header->dataLength;
                //处理网络消息
                OnNetMsg(header);
                //将消息缓冲区剩余未处理数据前移
                memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
                _lastPos = nSize;
            }
            else
            {
                //消息缓冲区剩余数据不够一条完整消息
                break;
            }
        }
        return 0;
    }

    //响应网络消息
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN_RESULT:
            {
                //LoginResult* Login = (LoginResult*)header;
                //printf("<socket=%d>收到服务器消息请求：CMD_LOGIN_RESULT  数据长度：%d\n", (int)_sock, Login->dataLength);
            }
            break;
            case CMD_LOGOUT_RESULT:
            {
                //LogoutResult* logout = (LogoutResult*)header;
                //printf("<socket=%d>收到服务器消息请求：CMD_LOGOUT_RESULT  数据长度：%d\n", (int)_sock, logout->dataLength);
            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                //NewUserJoin* userJoin = (NewUserJoin*)header;
                //printf("<socket=%d>收到服务器消息请求：CMD_NEW_USER_JOIN  数据长度：%d\n", (int)_sock, userJoin->dataLength);
            }
            break;
            case CMD_ERROR:
            {
                printf("<socket=%d>收到服务器消息请求：CMD_ERROR  数据长度：%d\n", (int)_sock, header->dataLength);
            }
            break;
            default:
            {
                printf("<socket=%d>收到未定义消息  数据长度：%d\n", (int)_sock, header->dataLength);
            }
        }
    }

    //关闭Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            closesocket(_sock);
            //清除Windows socket环境
            WSACleanup();
#else
            close(_sock);
#endif
            _sock = INVALID_SOCKET;
        }
        _isConnect = false;
    }

    //是否工作中
    bool isRun()
    {
        return INVALID_SOCKET != _sock && _isConnect;
    }
};

#endif //_EasyTcpClient_hpp_
