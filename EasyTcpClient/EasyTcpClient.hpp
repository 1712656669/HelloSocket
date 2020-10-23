#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
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

class EasyTcpClient
{
private:
    SOCKET _sock;

public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
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
            printf("建立<socket=%d>成功...\n", (int)_sock);
        }
        return _sock;
    }

    //连接服务器
    int Connect(const char* ip, unsigned short port)
    {
        //if (INVALID_SOCKET == _sock)
        //{
        //    InitSocket();
        //}
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
            printf("<socket=%d>连接服务器<%s, %d>成功...\n", (int)_sock, ip, port);
        }
        return ret;
    }

    //处理网络消息
    int _nCount = 0;
    bool OnRun()
    {
        if (isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock, &fdReads);
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_sock + 1, &fdReads, 0, 0, &t);
            printf("select ret=%d count=%d\n", ret, _nCount++);
            if (ret < 0)
            {
                printf("<socket=%d>select任务结束1\n", (int)_sock);
                Close();
                return false;
            }
            if (FD_ISSET(_sock, &fdReads))
            {
                FD_CLR(_sock, &fdReads);
                if (-1 == RecvData(_sock))
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

    //第二缓冲区，双缓冲
    char szRecv[4096] = {};

    //接收数据 处理粘包 拆分包
    int RecvData(SOCKET _cSock)
    {
        //接收服务端数据
        int nlen = (int)recv(_cSock, szRecv, 4096, 0); //sizeof(DataHeader)
        printf("nlen=%d\n", nlen);
        /*
        DataHeader* header = (DataHeader*)szRecv;
        if (nlen <= 0)
        {
            printf("<socket=%d>与服务器断开连接，任务结束。\n", (int)_sock);
            return -1;
        }
        //地址偏移sizeof(DataHeader)
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        OnNetMsg(header);
        */
        return 0;
    }

    //响应网络消息
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN_RESULT:
            {
                LoginResult* login = (LoginResult*)header;
                printf("<socket=%d>收到服务器消息请求：CMD_LOGIN_RESULT  数据长度：%d\n", (int)_sock, login->dataLength);
            }
            break;
            case CMD_LOGOUT_RESULT:
            {
                LogoutResult* logout = (LogoutResult*)header;
                printf("<socket=%d>收到服务器消息请求：CMD_LOGOUT_RESULT  数据长度：%d\n", (int)_sock, logout->dataLength);
            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* userJoin = (NewUserJoin*)header;
                printf("<socket=%d>收到服务器消息请求：CMD_NEW_USER_JOIN  数据长度：%d\n", (int)_sock, userJoin->dataLength);
            }
            break;
        }
    }

    //发送数据
    int SendData(DataHeader* header)
    {
        if (isRun() && header)
        {
            return (int)send(_sock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
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
    }

    //是否工作中
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }
};

#endif //_EasyTcpClient_hpp_
