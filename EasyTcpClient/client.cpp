#define FD_SETSIZE    10000
#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
#define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib,"ws2_32.lib")

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
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

    while (true)
    {
        // 3 输入请求命令
        char cmdBuf[128] = {};
        scanf("%s", cmdBuf);
        // 4 处理请求命令
        if (0 == strcmp(cmdBuf, "exit"))
        {
            printf("收到exit命令，任务结束。\n");
            break;
        }
        else if (0 == strcmp(cmdBuf, "login"))
        {
            // 5 向服务器发送请求命令
            Login login;
            strcpy(login.userName, "tao");
            strcpy(login.PassWord, "mm");
            send(_sock, (const char*)&login, sizeof(Login), 0);
            // 6 接收服务器返回的数据
            LoginResult loginRet = {};
            recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);
            printf("LoginResult: %d\n", loginRet.result);
        }
        else if (0 == strcmp(cmdBuf, "logout"))
        {
            Logout logout;
            strcpy(logout.userName, "tao");
            // 5 向服务器发送请求命令
            send(_sock, (const char*)&logout, sizeof(Logout), 0);
            // 6 接收服务器返回的数据
            LogoutResult logoutRet = {};
            recv(_sock, (char*)&logoutRet, sizeof(LogoutResult), 0);
            printf("LogoutResult: %d\n", logoutRet.result);
        }
        else
        {
            printf("命令不支持，请重新输入。\n");
        }
    }
    // 7 关闭套接字closesocket
    closesocket(_sock);
    //------------
    //清除Windows socket环境
    WSACleanup();
    printf("客户端已退出。\n");
    getchar();
    getchar();
    return 0;
}