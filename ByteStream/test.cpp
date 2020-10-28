#include "EasyTcpClient.hpp"
#include "CELLStream.hpp"
#include "CELLMsgStream.hpp"

class MyClient :public EasyTcpClient
{
public:
    //响应网络消息
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
        case CMD_LOGOUT_RESULT:
        {
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
        }
        break;

        case CMD_ERROR:
        {
            CELLLog::Info("<socket=%d>收到服务器消息请求：CMD_ERROR  数据长度：%d\n", (int)(_pClient->sockfd()), header->dataLength);
        }
        break;
        default:
        {
            CELLLog::Info("<socket=%d>收到未定义消息  数据长度：%d\n", (int)(_pClient->sockfd()), header->dataLength);
        }
        }
    }
};

int main()
{
    CELLSendStream s;
    s.setNetCmd(CMD_LOGOUT);
    s.WriteInt8(1);
    s.WriteInt16(2);
    s.WriteInt32(3);
    s.WriteFloat(4.5f);
    s.WriteDouble(6.7);
    s.WriteString("client");
    char a[] = "world";
    s.WriteArray(a, strlen(a));
    int b[] = { 1,2,3,4,5 };
    s.WriteArray(b, 5);
    s.finish();

    MyClient client;
    client.Connect("127.0.0.1", 4567);
    
    while (client.isRun())
    {
        client.OnRun();
        client.SendData(s.data(), s.length());
        CELLThread::Sleep(10);
    }

    return 0;
}