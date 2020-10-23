#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
//或者 项目-》属性-》C/C++ -》常规-》SDL检查 设置为否（配置：所有配置、平台：所有平台）
#define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <thread>

#pragma comment(lib,"ws2_32.lib")

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

int processor(SOCKET _cSock)
{
    //缓冲区
    char szRecv[4096] = {};
    // 5 接收服务端数据
    int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nlen <= 0)
    {
        printf("与服务器断开连接，任务结束。\n");
        return -1;
    }
    switch (header->cmd)
    {
        case CMD_LOGIN_RESULT:
        {
            //地址偏移sizeof(DataHeader)
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult*)szRecv;
            printf("收到服务器消息请求：CMD_LOGIN_RESULT  数据长度：%d\n", login->dataLength);
        }
        break;
        case CMD_LOGOUT_RESULT:
        {
            //地址偏移sizeof(DataHeader)
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LogoutResult* logout = (LogoutResult*)szRecv;
            printf("收到服务器消息请求：CMD_LOGOUT_RESULT  数据长度：%d\n", logout->dataLength);
        }
        break;
        case CMD_NEW_USER_JOIN:
        {
            //地址偏移sizeof(DataHeader)
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            NewUserJoin* userJoin = (NewUserJoin*)szRecv;
            printf("收到服务器消息请求：CMD_NEW_USER_JOIN  数据长度：%d\n", userJoin->dataLength);
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
bool g_bRun = true;
void cmdThread(SOCKET _sock)
{
    while (true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            printf("退出cmdThread线程\n");
            break;
        }
        else if (0 == strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "tao");
            strcpy(login.PassWord, "mm");
            send(_sock, (const char*)&login, sizeof(Login), 0);

            LoginResult loginRet = {};
            recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);
            printf("LoginResult: %d\n", loginRet.result);
        }
        else if (0 == strcmp(cmdBuf, "logout"))
        {
            Logout logout;
            strcpy(logout.userName, "tao");
            send(_sock, (const char*)&logout, sizeof(Logout), 0);

            LogoutResult logoutRet = {};
            recv(_sock, (char*)&logoutRet, sizeof(LogoutResult), 0);
            printf("LogoutResult: %d\n", logoutRet.result);
        }
        else
        {
            printf("命令不支持，请重新输入。\n");
        }
    }
}

int main()
{
    //启动Windows socket 2.x环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
    //------------
    //-- 用Socket API建立简易TCP客户端
    // 1 建立一个socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == _sock)
    {
        printf("错误，建立Socket失败...\n");
    }
    else
    {
        printf("建立Socket成功...\n");
    }
    // 2 连接服务器 connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567); //host to net unsigned short
    _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
    if (SOCKET_ERROR == ret)
    {
        printf("错误，连接服务器失败...\n");
    }
    else
    {
        printf("连接服务器成功...\n");
    }

    //启动线程
    std::thread t1(cmdThread, _sock);
    t1.detach();

    while (g_bRun)
    {
        fd_set fdReads;
        FD_ZERO(&fdReads);
        FD_SET(_sock, &fdReads);
        timeval t = { 1,0 }; //s,ms
        int ret = select((int)_sock, &fdReads, 0, 0, &t);
        if (ret < 0)
        {
            printf("select任务结束1\n");
            break;
        }
        if (FD_ISSET(_sock, &fdReads))
        {
            FD_CLR(_sock, &fdReads);

            if (-1 == processor(_sock))
            {
                printf("select任务结束2\n");
                break;
            }
        }
        //printf("空闲时间处理其他业务...\n");
    }
    // 7 关闭套接字closesocket
    closesocket(_sock);
    //------------
    //清除Windows socket环境
    WSACleanup();
    printf("客户端已退出\n");
    getchar();
    getchar();
    return 0;
}