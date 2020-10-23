// linux环境编译命令
// g++ client.cpp -std=c++11 -pthread -o client
// ./client

#include "EasyTcpClient.hpp"
#include <thread>

void cmdThread(EasyTcpClient* client)
{
    while (true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            client->Close();
            printf("退出cmdThread线程\n");
            break;
        }
        else if (0 == strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "tao");
            strcpy(login.PassWord, "mm");
            client->SendData(&login);
        }
        else if (0 == strcmp(cmdBuf, "logout"))
        {
            Logout logout;
            strcpy(logout.userName, "tao");
            client->SendData(&logout);
        }
        else
        {
            printf("命令不支持，请重新输入。\n");
        }
    }
}

int main()
{
    EasyTcpClient client1;
    client1.Connect("192.168.56.1", 4567);

    EasyTcpClient client2;
    client2.Connect("192.168.0.127", 4567);

    EasyTcpClient client3;
    client3.Connect("192.168.0.128", 4567);

    //启动线程
    /*std::thread t1(cmdThread, &client1);
    t1.detach();

    std::thread t2(cmdThread, &client2);
    t2.detach();

    std::thread t3(cmdThread, &client3);
    t3.detach();*/

    Login login;
    strcpy(login.userName, "tao");
    strcpy(login.PassWord, "mm");
    while (client1.isRun()) // || client2.isRun() || client3.isRun()
    {
        client1.OnRun();
        client2.OnRun();
        client3.OnRun();

        client1.SendData(&login);
        client2.SendData(&login);
        client3.SendData(&login);
        //printf("空闲时间处理其他业务...\n");
    }
    client1.Close();
    /*client2.Close();
    client3.Close();*/

    printf("客户端已退出\n");
    //getchar();
    //getchar();
    return 0;
}