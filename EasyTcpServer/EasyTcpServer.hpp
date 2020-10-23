#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

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
#include <vector>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif

class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }

    void setLastPos(int pos)
    {
        _lastPos = pos;
    }

private:
    SOCKET _sockfd; //fd_set file desc set
    //第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE * 10];
    //消息缓冲区的数据尾部位置
    int _lastPos;
};

class EasyTcpServer
{
private:
    SOCKET _sock;
    std::vector<ClientSocket*> _clients;
    CELLTimestamp _tTime;
    int _recvCount;

public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
    }

    virtual ~EasyTcpServer()
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

    //绑定IP和端口号
    int Bind(const char* ip, unsigned short port)
    {
        if (INVALID_SOCKET == _sock)
         {
         InitSocket();
         }
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET; //地址族(IPV4)
        _sin.sin_port = htons(4567); //端口号，host to net unsigned short

#ifdef _WIN32
        if (ip)
        {
            _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //本机地址
        }
        else
        {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
        }
#else
        if (ip)
        {
            _sin.sin_addr.s_addr = inet_addr("127.0.0.1"); //本机地址
        }
        else
        {
            _sin.sin_addr.s_addr = INADDR_ANY;
        }
#endif
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("错误，绑定网络端口<%d>失败...\n", port);
        }
        else
        {
            printf("绑定网络端口<%d>成功...\n", port);
        }
        return ret;
    }

    //监听端口号
    int Listen(int n) //等待连接队列的最大长度
    {
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>错误，监听网络端口失败\n", (int)_sock);
        }
        else
        {
            printf("<socket=%d>监听网络端口成功...\n", (int)_sock);
        }
        return ret;
    }

    //接受客户端连接
    SOCKET Accept()
    {
        sockaddr_in clientAddr = {};
        int nAddrLen = sizeof(sockaddr_in);
        SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
        cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if (INVALID_SOCKET == cSock)
        {
            printf("<socket=%d>错误，接受到无效客户端SOCKET...\n", (int)_sock);
        }
        else
        {
            //NewUserJoin userJoin;
            //SendDataToAll(&userJoin);
            _clients.push_back(new ClientSocket(cSock));
            printf("<socket=%d>新客户<%d>加入：socket = %d，IP = %s\n", (int)_sock, (int)_clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
        }
        return cSock;
    }

    //关闭Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                closesocket(_clients[n]->sockfd());
                delete _clients[n];
            }
            //关闭套接字closesocket
            closesocket(_sock);
            //清除Windows socket环境
            WSACleanup();
#else
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                close(_clients[n]->sockfd());
                delete _clients[n];
            }
            close(_sock);
#endif
            _clients.clear();
        }
    }

    //处理网络消息
    int _nCount = 0;
    bool OnRun()
    {
        if (isRun())
        {
            //伯克利套接字 BSD socket
            fd_set fdRead; //select监视的可读文件句柄集合
            fd_set fdWrite; //select监视的可写文件句柄集合
            fd_set fdExp; //select监视的异常文件句柄集合
            //将fd_set清零使集合中不含任何SOCKET
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExp);
            //将SOCKET加入fd_set集合
            FD_SET(_sock, &fdRead);
            FD_SET(_sock, &fdWrite);
            FD_SET(_sock, &fdExp);

            SOCKET maxSock = _sock;
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                FD_SET(_clients[n]->sockfd(), &fdRead);
                if (maxSock < _clients[n]->sockfd())
                {
                    maxSock = _clients[n]->sockfd();
                }
            }
            //int nfds 集合中所有文件描述符的范围，而不是数量
            //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
            //timeout 本次select()的超时结束时间
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
            //printf("select ret=%d count=%d\n", ret, _nCount++);
            if (ret < 0)
            {
                printf("select任务结束。\n");
                Close();
                return false;
            }
            //判断描述符(socket)是否在集合中
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
                //return true;
            }
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
                {
                    if (-1 == RecvData(_clients[n]))
                    {
                        //std::vector<SOCKET>::iterator
                        auto iter = _clients.begin() + n;
                        if (iter != _clients.end())
                        {
                            delete _clients[n];
                            _clients.erase(iter);
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    //是否工作中
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }

    //第二缓冲区，双缓冲
    char _szRecv[RECV_BUFF_SIZE] = {};

    //接收数据 处理粘包 拆分包
    int RecvData(ClientSocket* pClient)
    {
        //接收客户端数据
        int nlen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0); //sizeof(DataHeader)
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            printf("客户端<Socket=%d>已退出，任务结束。\n", (int)(pClient->sockfd()));
            return -1;
        }
        //将收取收取到的数据拷贝到消息缓冲区
        memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nlen);
        //消息缓冲区的数据尾部位置后移
        pClient->setLastPos(pClient->getLastPos() + nlen);
        //判断消息缓冲区的数据长度大于消息头DataHeader长度
        while (pClient->getLastPos() >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息的长度
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            //判断消息缓冲区的数据长度大于消息长度
            if (pClient->getLastPos() >= header->dataLength)
            {
                //剩余未处理消息缓冲区消息的长度
                int nSize = pClient->getLastPos() - header->dataLength;
                //处理网络消息
                OnNetMsg(pClient->sockfd(), header);
                //将消息缓冲区剩余未处理数据前移
                memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
                pClient->setLastPos(nSize);
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
    void OnNetMsg(SOCKET cSock, DataHeader* header)
    {
        _recvCount++;
        auto t1 = _tTime.getElapseSecond();
        if (t1 >= 1.0)
        {
            printf("time<%lf>, socket<%d>, clients<%d>, recvCount<%d>\n", t1, (int)_sock, (int)_clients.size(), _recvCount);
            _recvCount = 0;
            _tTime.update();
        }
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                //printf("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", cSock, login->dataLength, login->userName, login->PassWord);
                //忽略判断用户密码是否正确的过程
                //LoginResult ret;
                //SendData(cSock, &ret);
            }
            break;
            case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                //printf("收到客户端<Socket=%d>请求：CMD_LOGOUT  数据长度：%d  用户名：%s\n", cSock, logout->dataLength, logout->userName);
                //忽略判断用户密码是否正确的过程
                //LogoutResult ret;
                //SendData(cSock, &ret);
            }
            break;
            default:
            {
                printf("<socket=%d>收到未定义消息  数据长度：%d\n", (int)_sock, header->dataLength);
                //DataHeader ret;
                //SendData(cSock, &ret);
            }
        }
    }

    //发送指定Socket数据
    int SendData(SOCKET cSock, DataHeader* header)
    {
        if (isRun() && header)
        {
            return (int)send(cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

    void SendDataToAll(DataHeader* header)
    {
        for (int n = (int)_clients.size() - 1; n >= 0; n--)
        {
            SendData(_clients[n]->sockfd(), header);
        }
    }
};

#endif //_EasyTcpServer_hpp_
