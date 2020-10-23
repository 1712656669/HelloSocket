#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
    #include <windows.h>
    #include <WinSock2.h>
    //静态链接库，解析WSAStartup和WSACleanup
    #pragma comment(lib,"ws2_32.lib")
#else
    // linux环境编译命令
    // g++ server.cpp -std=c++11 -pthread -o server
    // ./server
    #include <unistd.h> //unix std，类似windows.h
    #include <arpa/inet.h> //类似WinSock2.h
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <vector>

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};

struct DataHeader
{
    short dataLength; //数据长度
    short cmd; //命令
};

//DataPackage
struct Login :public DataHeader
{
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
        *userName = '\0';
        *PassWord = '\0';
    }
    char userName[32];
    char PassWord[32];
};

struct LoginResult :public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;
};

struct Logout :public DataHeader
{
    Logout()
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
        *userName = '\0';
    }
    char userName[32];
};

struct LogoutResult :public DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 0;
    }
    int result;
};

struct NewUserJoin :public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        sock = 0;
    }
    int sock;
};

std::vector<SOCKET> g_clients;

int processor(SOCKET _cSock)
{
    //缓冲区
    char szRecv[4096] = {};
    //接收服务端数据
    int nlen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nlen <= 0)
    {
        printf("客户端<Socket=%d>已退出，任务结束。\n", (int)_cSock);
        return -1;
    }
    switch (header->cmd)
    {
        case CMD_LOGIN:
        {
            //地址偏移sizeof(DataHeader)
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Login* login = (Login*)szRecv;
            printf("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", (int)_cSock, login->dataLength, login->userName, login->PassWord);
            //忽略判断用户密码是否正确的过程
            LoginResult ret;
            send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
        }
        break;
        case CMD_LOGOUT:
        {
            //地址偏移sizeof(DataHeader)
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Logout* logout = (Logout*)szRecv;
            printf("收到客户端<Socket=%d>请求：CMD_LOGOUT  数据长度：%d  用户名：%s\n", (int)_cSock, logout->dataLength, logout->userName);
            //忽略判断用户密码是否正确的过程
            LogoutResult ret;
            send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
        }
        break;
        default:
        {
            DataHeader header = { 0,CMD_ERROR };
            send(_cSock, (char*)&header, sizeof(DataHeader), 0);
        }
    }
    return 0;
}

int main()
{
#ifdef _WIN32
    //启动Windows socket 2.x环境
    WORD ver = MAKEWORD(2, 2); //版本号
    WSADATA dat; //数据指针
    WSAStartup(ver, &dat);
#endif

    //建立一个socket套接字
    //地址族(IPV4)、类型（面向数据流）、传输协议(TCP)
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == _sock)
    {
        printf("错误，建立Socket失败...\n");
    }
    else
    {
        printf("建立Socket成功...\n");
    }

    //bind 绑定用于接受客户端连接的网络端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET; //地址族(IPV4)
    _sin.sin_port = htons(4567); //端口号，host to net unsigned short
#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = INADDR_ANY; //ip地址，inet_addr("127.0.0.1")（本机地址）
#else
    _sin.sin_addr.s_addr = INADDR_ANY;
#endif
    if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
    {
        printf("错误，绑定用于接受客户端连接的网络端口失败\n");
    }
    else
    {
        printf("绑定网络端口成功...\n");
    }
    //listen 监听网络端口
    if (SOCKET_ERROR == listen(_sock, 5)) //第二个参数是等待连接队列的最大长度
    {
        printf("错误，监听网络端口失败\n");
    }
    else
    {
        printf("监听网络端口成功...\n");
    }

    while (true)
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
        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            FD_SET(g_clients[n], &fdRead);
            if (maxSock < g_clients[n])
            {
                maxSock = g_clients[n];
            }
        }
        //int nfds 集合中所有文件描述符的范围，而不是数量
        //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
        //timeout 本次select()的超时结束时间
        timeval t = { 1,0 }; //s,ms
        int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
        if (ret < 0)
        {
            printf("select任务结束。\n");
            break;
        }
        //判断描述符(socket)是否在集合中
        if (FD_ISSET(_sock, &fdRead))
        {
            FD_CLR(_sock, &fdRead);
            //accept 等待接受客户端连接
            sockaddr_in clientAddr = {};
            int nAddrLen = sizeof(sockaddr_in);
            SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
            _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
            _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
            if (INVALID_SOCKET == _cSock)
            {
                printf("错误，接受到无效客户端SOCKET...\n");
            }
            else
            {
                for (int n = (int)g_clients.size() - 1; n >= 0; n--)
                {
                    NewUserJoin userJoin;
                    send(g_clients[n], (const char*)&userJoin, sizeof(NewUserJoin), 0);
                }
                g_clients.push_back(_cSock);
                printf("新客户加入：socket = %d，IP = %s\n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
            }
        }
        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            if (FD_ISSET(g_clients[n], &fdRead))
            {
                if (-1 == processor(g_clients[n]))
                {
                    //std::vector<SOCKET>::iterator
                    auto iter = g_clients.begin() + n;
                    if (iter != g_clients.end())
                    {
                        g_clients.erase(iter);
                    }
                }
            }
        }
        //printf("空闲时间处理其他业务...\n");
    }

#ifdef _WIN32
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        closesocket(g_clients[n]);
    }
    //关闭套接字closesocket
    closesocket(_sock);
    //清除Windows socket环境
    WSACleanup();
#else
    for (int n = (int)g_clients.size() - 1; n >= 0; n--)
    {
        close(g_clients[n]);
    }
    close(_sock);
#endif
    printf("服务器已退出\n");
    getchar();
    return 0;
}
