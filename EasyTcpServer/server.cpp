// linux环境编译命令
// g++ server.cpp -std=c++11 -pthread -o server
// ./server

#include "EasyTcpServer.hpp"
#include <thread>

bool g_bRun = true;
void cmdThread()
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
        else
        {
            printf("命令不支持，请重新输入。\n");
        }
    }
}

int main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);

    //启动线程
    std::thread t1(cmdThread);
    t1.detach();

    while (g_bRun)
    {
        server.OnRun();
        //printf("空闲时间处理其他业务...\n");
    }
    server.Close();
    printf("服务器已退出\n");
    getchar();
    getchar();
    return 0;
}
