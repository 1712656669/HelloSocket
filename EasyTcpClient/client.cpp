// linux环境编译命令
// g++ client.cpp -std=c++11 -pthread -o client
// ./client

#include "EasyTcpClient.hpp"
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
    const int cCount = 100; //最大连接数-服务端个数FD_SETSIZE - 1
    EasyTcpClient* client[cCount];
    for (int n = 0; n < cCount; n++)
    {
        if (!g_bRun)
        {
            return 0;
        }
        client[n] = new EasyTcpClient();
    }
    for (int n = 0; n < cCount; n++)
    {
        if (!g_bRun)
        {
            return 0;
        }
        client[n]->InitSocket();
        client[n]->Connect("127.0.0.1", 4567);
        printf("Connect=%d\n", n);
    }
 
    //启动线程
    std::thread t1(cmdThread);
    t1.detach();

    Login login;
    strcpy(login.userName, "tao");
    strcpy(login.PassWord, "mm");
    while (g_bRun)
    {
        for (int n = 0; n < cCount; n++)
        {
            client[n]->SendData(&login);
            client[n]->OnRun();
        }
        //printf("空闲时间处理其他业务...\n");
    }

    for (int n = 0; n < cCount; n++)
    {
        client[n]->Close();
    }

    printf("客户端已退出\n");
    getchar();
    getchar();
    return 0;
}
