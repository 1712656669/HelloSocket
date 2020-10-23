#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
//或者 项目-》属性-》C/C++ -》常规-》SDL检查 设置为否（配置：所有配置、平台：所有平台）

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

//静态链接库，解析WSAStartup和WSACleanup
//#pragma comment(lib,"ws2_32.lib")
//以上方法在其他平台不支持，可采用以下统一方法
//项目-》属性-》链接器-》输入-》附加依赖项 中添加ws2_32.lib（配置：所有配置、平台：所有平台）

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
    WORD ver = MAKEWORD(2, 2); //版本号
    WSADATA dat; //数据指针
    WSAStartup(ver, &dat);
    //------------

    //-- 用Socket API建立简易TCP服务端
    // 1 建立一个socket套接字
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
    // 2 bind 绑定用于接受客户端连接的网络端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET; //地址族(IPV4)
    _sin.sin_port = htons(4567); //端口号，host to net unsigned short
    _sin.sin_addr.S_un.S_addr = INADDR_ANY; //ip地址，inet_addr("127.0.0.1")（本机地址）
    if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))
    {
        printf("错误，绑定用于接受客户端连接的网络端口失败\n");
    }
    else
    {
        printf("绑定网络端口成功...\n");
    }
    // 3 listen 监听网络端口
    if (SOCKET_ERROR == listen(_sock, 5)) //第二个参数是等待连接队列的最大长度
    {
        printf("错误，监听网络端口失败\n");
    }
    else
    {
        printf("监听网络端口成功...\n");
    }
    // 4 accept 等待接受客户端连接
    sockaddr_in clientAddr = {};
    int nAddrLen = sizeof(sockaddr_in);
    SOCKET _cSock = INVALID_SOCKET;

    _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
    if (INVALID_SOCKET == _cSock)
    {
        printf("错误，接受到无效客户端SOCKET...\n");
    }
    printf("新客户加入：socket = %d，IP = %s\n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));

    char _recvBuf[128] = {};
    while (true)
    {
        DataHeader header = {};
        // 5 接收服务端数据
        int nlen = recv(_cSock, (char*)&header, sizeof(DataHeader), 0);
        if (nlen <= 0)
        {
            printf("客户端已退出，任务结束。\n");
            break;
        }
        switch (header.cmd)
        {
            case CMD_LOGIN:
            {
                Login login = {};
                recv(_cSock, (char*)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
                printf("收到命令：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", login.dataLength, login.userName, login.PassWord);
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
            }
            break;
            case CMD_LOGOUT:
            {
                Logout logout = {};
                recv(_cSock, (char*)&logout + sizeof(DataHeader), sizeof(Logout) - sizeof(DataHeader), 0);
                printf("收到命令：CMD_LOGIN  数据长度：%d  用户名：%s\n", logout.dataLength, logout.userName);
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                send(_cSock, (char*)&header, sizeof(DataHeader), 0);
                send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
            }
            break;
            default:
                header.cmd = CMD_ERROR;
                header.dataLength = 0;
                send(_cSock, (char*)&header, sizeof(DataHeader), 0);
        }
    }

    // 8 关闭套接字closesocket
    closesocket(_sock);
    //------------
    //清除Windows socket环境
    WSACleanup();
    printf("服务器已退出。\n");
    getchar();
    return 0;
}