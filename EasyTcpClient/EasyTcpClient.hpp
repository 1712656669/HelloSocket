#ifndef _EASY_TCP_CLIENT_HPP_
#define _EASY_TCP_CLIENT_HPP_

#include "CELL.hpp"
#include "MessageHeader.hpp"
#include "CELLNetWork.hpp"
#include "CELLClient.hpp"

class EasyTcpClient
{
public:
    EasyTcpClient()
    {
        _pClient = nullptr;
        _isConnect = false;
    }

    virtual ~EasyTcpClient()
    {
        Close();
    }

    //初始化Socket
    SOCKET InitSocket()
    {
        CELLNetWork::Init();
        if (_pClient)
        {
            printf("<socket=%d>关闭旧连接...\n", _pClient->sockfd());
            Close();
        }
        SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("<socket=%d>错误，建立Socket失败...\n", (int)_sock);
        }
        else
        {
            //printf("建立<socket=%d>成功...\n", (int)_sock);
            _pClient = new CELLClient(_sock);
        }
        return _sock;
    }

    //连接服务器
    int Connect(const char* ip, unsigned short port)
    {
        if (!_pClient)
        {
            InitSocket();
        }
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port); //host to net unsigned short
#ifdef _WIN32
        _sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
        _sin.sin_addr.s_addr = inet_addr(ip);
#endif
        int ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>错误，连接服务器<%s, %d>失败...\n", (int)(_pClient->sockfd()), ip, port);
        }
        else
        {
            _isConnect = true;
            //printf("<socket=%d>连接服务器<%s, %d>成功...\n", (int)_sock, ip, port);
        }
        return ret;
    }

    //发送数据
    int SendData(DataHeader* header)
    {
        return _pClient->SendData(header);
    }

    //处理网络消息
    bool OnRun()
    {
        if (isRun())
        {
            SOCKET sock = _pClient->sockfd();
            fd_set fdRead;
            FD_ZERO(&fdRead);
            FD_SET(sock, &fdRead);

            fd_set fdWrite;
            FD_ZERO(&fdWrite);

            timeval t = { 0,1 }; //s,us
            int ret = 0;
            if (_pClient->needwrite())
            {
                FD_SET(sock, &fdWrite);
                ret = select((int)sock + 1, &fdRead, &fdWrite, nullptr, &t);
            }
            else
            {
                ret = select((int)sock + 1, &fdRead, nullptr, nullptr, &t);
            }
            
            if (ret < 0) //select错误
            {
                printf("<socket=%d>select任务结束1\n", (int)sock);
                Close();
                return false;
            }
            if (FD_ISSET(sock, &fdRead))
            {
                if (-1 == RecvData(sock)) //与服务器断开连接
                {
                    printf("<socket=%d>select任务结束2\n", (int)sock);
                    Close();
                    return false;
                }
            }
            if (FD_ISSET(sock, &fdWrite))
            {
                if (-1 == _pClient->SendDataReal()) //与服务器断开连接
                {
                    printf("<socket=%d>select任务结束2\n", (int)sock);
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    //接收数据 处理粘包 拆分包
    int RecvData(SOCKET cSock)
    {
        //接收客户端数据
        int nLen = _pClient->RecvData();
        if (nLen > 0)
        {
            //循环 判断数据是否有消息需要处理
            while (_pClient->hasMsg())
            {
                //处理网络消息
                OnNetMsg(_pClient->front_msg());
                //移除消息队列（缓冲区）最前的一条数据
                _pClient->pop_front_msg();
            }
        }
        return nLen;
    }

    //响应网络消息
    virtual void OnNetMsg(DataHeader* header) = 0;

    //关闭Socket
    void Close()
    {
        if (_pClient)
        {
            delete _pClient;
            _pClient = nullptr;
        }
        _isConnect = false;
    }

    //是否工作中
    bool isRun()
    {
        return _pClient && _isConnect;
    }

protected:
    CELLClient* _pClient;
    bool _isConnect;
};

#endif // !_EASY_TCP_CLIENT_HPP_
