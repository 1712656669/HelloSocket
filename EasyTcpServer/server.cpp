// linux环境编译命令
// g++ server.cpp -std=c++11 -pthread -o server
// ./server

#include "EasyTcpServer.hpp"
#include "Alloctor.hpp"
#include <thread>

//void cmdThread()
//{
//    
//}

class MyServer :public EasyTcpServer
{
public:
    //只会被一个线程调用
    virtual void OnNetJoin(CELLClientPtr& pClient)
    {
        EasyTcpServer::OnNetJoin(pClient);
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetLeave(CELLClientPtr& pClient)
    {
        EasyTcpServer::OnNetLeave(pClient);
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetMsg(CELLServer* pCELLServer, CELLClientPtr& pClient, DataHeader* header)
    {
        EasyTcpServer::OnNetMsg(pCELLServer, pClient, header);
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                pClient->resetDTHeart(); //心跳时间重置
                //Login* Login = (Login*)header;
                //printf("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", pClient->sockfd(), Login->dataLength, Login->userName, Login->PassWord);
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                if (SOCKET_ERROR == pClient->SendData(&ret))
                {
                    //消息发送区满了，消息没发出去
                    printf("<socket=%d> Send Full\n", (int)(pClient->sockfd()));
                }
                //auto ret = std::make_shared<LoginResult>();
                //LoginResult* ret = new LoginResult();
                //pCELLServer->addSendTask(pClient, ret);
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
            case CMD_C2S_HEART:
            {
                pClient->resetDTHeart();
                C2S_Heart ret;
                pClient->SendData(&ret);
            }
            default:
            {
                printf("<socket=%d>收到未定义消息  数据长度：%d\n", (int)(pClient->sockfd()), header->dataLength);
                //DataHeader ret;
                //pClient->SendData(&ret);
            }
        }
    }

    virtual void OnNetRecv(CELLClientPtr pClient)
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
    /*std::thread t1(cmdThread);
    t1.detach();*/

    bool g_bRun = true;
    while (g_bRun)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            server.Close();
            printf("退出cmdThread线程\n");
            break;
        }
        else
        {
            printf("命令不支持，请重新输入。\n");
        }
    }
    
    printf("服务器已退出\n");

    /*
    CELLTaskServer task;
    task.Start();
    Sleep(100);
    task.Close();
    */

    /*while (true)
    {
        Sleep(100);
    }*/
    getchar();
    getchar();

    return 0;
}
