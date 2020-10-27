#include "EasyTcpClient.hpp"
#include "CELLTimestamp.hpp"
#include <thread>
#include <atomic>

class MyClient :public EasyTcpClient
{
public:
    //响应网络消息
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
        case CMD_LOGIN_RESULT:
        {
            //LoginResult* Login = (LoginResult*)header;
            //printf("<socket=%d>收到服务器消息请求：CMD_LOGIN_RESULT  数据长度：%d\n", (int)_sock, Login->dataLength);
        }
        break;
        case CMD_LOGOUT_RESULT:
        {
            //LogoutResult* logout = (LogoutResult*)header;
            //printf("<socket=%d>收到服务器消息请求：CMD_LOGOUT_RESULT  数据长度：%d\n", (int)_sock, logout->dataLength);
        }
        break;
        case CMD_NEW_USER_JOIN:
        {
            //NewUserJoin* userJoin = (NewUserJoin*)header;
            //printf("<socket=%d>收到服务器消息请求：CMD_NEW_USER_JOIN  数据长度：%d\n", (int)_sock, userJoin->dataLength);
        }
        break;
        case CMD_ERROR:
        {
            printf("<socket=%d>收到服务器消息请求：CMD_ERROR  数据长度：%d\n", (int)(_pClient->sockfd()), header->dataLength);
        }
        break;
        default:
        {
            printf("<socket=%d>收到未定义消息  数据长度：%d\n", (int)(_pClient->sockfd()), header->dataLength);
        }
        }
    }
};

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

//客户端数量
const int cCount = 100; //最大连接数 - 服务端个数 FD_SETSIZE - 1
//发送线程数量
const int tCount = 4;
EasyTcpClient* client[cCount];
std::atomic_int sendCount;
std::atomic_int readyCount;

void recvThread(int begin, int end)
{
    while (g_bRun)
    {
        for (int n = begin; n < end; n++)
        {
            client[n]->OnRun();
        }
        //printf("空闲时间处理其他业务...\n");
    }
}

void sendThread(int id)
{
    printf("thread<%d>, start\n", id);
    //4个线程 ID 1-4
    int c = cCount / tCount;
    int begin = (id - 1) * c;
    int end = id * c;

    for (int n = begin; n < end; n++)
    {
        client[n] = new MyClient();
    }
    for (int n = begin; n < end; n++)
    {
        client[n]->InitSocket();
        client[n]->Connect("127.0.0.1", 4567);
    }
    
    printf("thread<%d>, Connect=<begin=%d, end=%d>\n", id, begin, end - 1);

    readyCount++;
    while(readyCount < tCount)
    {
        //等待其他线程准备好发送数据
        std::chrono::milliseconds t(10);
        std::this_thread::sleep_for(t);
    }

    std::thread t1(recvThread, begin, end);
    t1.detach();

    Login login[10];
    for (int n = 0; n < 10; n++)
    {
        strcpy(login[n].userName, "tao");
        strcpy(login[n].PassWord, "mm");
    }

    while (g_bRun)
    {
        for (int n = begin; n < end; n++)
        {
            if (SOCKET_ERROR != client[n]->SendData(login))
            {
                sendCount++;
            }
        }
        //std::chrono::milliseconds t(999);
        //std::this_thread::sleep_for(t);
        //printf("空闲时间处理其他业务...\n");
    }

    for (int n = begin; n < end; n++)
    {
        client[n]->Close();
        delete client[n];
    }

    printf("thread<%d>, exit\n", id);
}

int main()
{
    //启动UI线程
    std::thread t1(cmdThread);
    t1.detach();

    //启动发送线程
    for (int n = 0; n < tCount; n++)
    {
        std::thread t1(sendThread, n + 1);
        t1.detach();
    }

    CELLTimestamp tTime;

    while (g_bRun)
    {
        auto t = tTime.getElapseSecond();
        if (t >= 1.0)
        {
            printf("thread<%d>, clients<%d>, time<%lf>, send<%d>\n", tCount, cCount, t, (int)(sendCount / t));
            sendCount = 0;
            tTime.update();
        }
    }

    printf("客户端已退出\n");
    getchar();
    getchar();
    return 0;
}
