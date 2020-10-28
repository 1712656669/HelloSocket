#include "EasyTcpServer.hpp"
#include "Alloctor.hpp"
#include "CELLMsgStream.hpp"
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
                //CELLLog::Info("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", pClient->sockfd(), Login->dataLength, Login->userName, Login->PassWord);
                //忽略判断用户密码是否正确的过程
                LoginResult ret;
                if (SOCKET_ERROR == pClient->SendData(&ret))
                {
                    //消息发送区满了，消息没发出去
                    CELLLog::Info("<socket=%d> Send Full\n", (int)(pClient->sockfd()));
                }
                //auto ret = std::make_shared<LoginResult>();
                //LoginResult* ret = new LoginResult();
                //pCELLServer->addSendTask(pClient, ret);
            }
            break;
            case CMD_LOGOUT:
            {;
                CELLRecvStream r(header);
                auto n1 = r.ReadInt8();
                auto n2 = r.ReadInt16();
                auto n3 = r.ReadInt32();
                auto n4 = r.ReadFloat();
                auto n5 = r.ReadDouble();

                uint32_t n = 0;
                r.onlyRead(n);
                char username[32] = {};
                auto n6 = r.ReadArray(username, 32);
                char password[32] = {};
                auto n7 = r.ReadArray(password, 32);
                int ndata[10] = {};
                auto n8 = r.ReadArray(ndata, 10);

                CELLSendStream s;
                s.setNetCmd(CMD_LOGOUT_RESULT);
                s.WriteInt8(1);
                s.WriteInt16(2);
                s.WriteInt32(3);
                s.WriteFloat(4.5f);
                s.WriteDouble(6.7);
                const char* str = "server";
                s.WriteArray(str, (uint32_t)strlen(str));
                char a[] = "world";
                s.WriteArray(a, (uint32_t)strlen(a));
                int b[] = { 1,2,3,4,5 };
                s.WriteArray(b, 5);
                s.finish();
                pClient->SendData(s.data(), s.length());
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
                CELLLog::Info("<socket=%d>收到未定义消息  数据长度：%d\n", (int)(pClient->sockfd()), header->dataLength);
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
    CELLLog::Instance().setLogPath("serverLog.txt", "w");
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
            break;
        }
        else
        {
            CELLLog::Info("命令不支持，请重新输入。\n");
        }
    }
    
    CELLLog::Info("服务器已退出\n");
    return 0;
}
