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
        _fdRead = new fd_set;
        _fdWrite = new fd_set;
    }

    virtual ~EasyTcpClient()
    {
        Close();
        delete _fdRead;
        delete _fdWrite;
    }

    //初始化Socket
    SOCKET InitSocket()
    {
        CELLNetWork::Init();
        if (_pClient)
        {
            CELLLog::Info("<socket=%d>关闭旧连接...\n", (int)(_pClient->sockfd()));
            Close();
        }
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == sock)
        {
            CELLLog::Info("<socket=%d>错误，建立Socket失败...\n", (int)sock);
        }
        else
        {
            //CELLLog::Info("建立<socket=%d>成功...\n", (int)sock);
            _pClient = new CELLClient(sock);
        }
        return sock;
    }

    //连接服务器
    int Connect(const char* ip, unsigned short port)
    {
        if (nullptr == _pClient)
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
            CELLLog::Info("<socket=%d>错误，连接服务器<%s, %d>失败...\n", (int)(_pClient->sockfd()), ip, port);
        }
        else
        {
            _isConnect = true;
            CELLLog::Info("<socket=%d>连接服务器<%s, %d>成功...\n", (int)(_pClient->sockfd()), ip, port);
        }
        return ret;
    }

    //发送数据
    int SendData(DataHeader* header)
    {
        if (isRun())
        {
            return _pClient->SendData(header);
        }
        return 0;
    }

    int SendData(const char* pData, int len)
    {
        if (isRun())
        {
            return _pClient->SendData(pData, len);
        }
        return 0;
    }

    //处理网络消息
    bool OnRun()
    {
        if (isRun())
        {
            SOCKET sock = _pClient->sockfd();
            FD_ZERO(_fdRead);
            FD_SET(sock, _fdRead);
            FD_ZERO(_fdWrite);

            timeval t = { 0,10 }; //s,us
            int ret = 0;
            if (_pClient->needwrite())
            {
                FD_SET(sock, _fdWrite);
                ret = select((int)sock + 1, _fdRead, _fdWrite, nullptr, &t);
            }
            else
            {
                ret = select((int)sock + 1, _fdRead, nullptr, nullptr, &t);
            }
            
            if (ret < 0) //select错误
            {
                CELLLog::Info("<socket=%d>select任务结束1\n", (int)sock);
                Close();
                return false;
            }
            if (FD_ISSET(sock, _fdRead))
            {
                if (-1 == RecvData(sock)) //与服务器断开连接
                {
                    CELLLog::Info("<socket=%d>select任务结束2\n", (int)sock);
                    Close();
                    return false;
                }
            }
            if (FD_ISSET(sock, _fdWrite))
            {
                if (-1 == _pClient->SendDataReal()) //与服务器断开连接
                {
                    CELLLog::Info("<socket=%d>select任务结束2\n", (int)sock);
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
        if (isRun())
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
        return 0;
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
private:
    fd_set* _fdRead;
    fd_set* _fdWrite;
};

#endif // !_EASY_TCP_CLIENT_HPP_
