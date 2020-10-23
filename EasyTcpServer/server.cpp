// linux环境编译命令
// g++ server.cpp -std=c++11 -pthread -o server
// ./server

#include "EasyTcpServer.hpp"
#include "Alloctor.hpp"
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

class MyServer :public EasyTcpServer
{
public:
    //只会被一个线程调用
    virtual void OnNetJoin(CellClientPtr& pClient)
    {
        EasyTcpServer::OnNetJoin(pClient);
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetLeave(CellClientPtr& pClient)
    {
        EasyTcpServer::OnNetLeave(pClient);
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetMsg(CellServer* pCellServer, CellClientPtr& pClient, DataHeaderPtr& header)
    {
        EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                //Login* Login = (Login*)header;
                //printf("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", pClient->sockfd(), Login->dataLength, Login->userName, Login->PassWord);
                //忽略判断用户密码是否正确的过程
                //LoginResult ret;
                //pClient->SendData(&ret);
                auto ret = std::make_shared<LoginResult>();
                pCellServer->addSendTask(pClient, (DataHeaderPtr)ret);
            }
            break;
            case CMD_LOGOUT:
            {
                //Logout* logout = (Logout*)header;
                //printf("收到客户端<Socket=%d>请求：CMD_LOGOUT  数据长度：%d  用户名：%s\n", pClient->sockfd(), logout->dataLength, logout->userName);
                //忽略判断用户密码是否正确的过程
                //LogoutResult ret;
                //pClient->SendData(&ret);
            }
            break;
            default:
            {
                printf("<socket=%d>收到未定义消息  数据长度：%d\n", (int)(pClient->sockfd()), header->dataLength);
                //DataHeader ret;
                //pClient->SendData(&ret);
            }
        }
    }

    virtual void OnNetRecv(CellClientPtr pClient)
    {
        EasyTcpServer::OnNetRecv(pClient);
    }
};

int main()
{
    MyServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);
    server.Start(4);

    //启动UI线程
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
