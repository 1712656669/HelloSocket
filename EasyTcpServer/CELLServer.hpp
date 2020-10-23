#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "CELL.hpp"
#include "INetEvent.hpp"
#include "CELLClient.hpp"

#include <vector>
#include <map>

//网络消息发送任务
class CellS2CTask :public CellTask
{
private:
    CellClientPtr _pClient;
    DataHeaderPtr _pHeader;

public:
    CellS2CTask(CellClientPtr pClient, DataHeaderPtr pHeader)
    {
        _pClient = pClient;
        _pHeader = pHeader;
    }

    //执行任务
    void doTask()
    {
        _pClient->SendData(_pHeader);
    }

};

//网络消息接收处理服务类
class CellServer
{
public:
    CellServer(SOCKET sock = INVALID_SOCKET)
    {
        _sock = sock;
        _pNetEvent = nullptr;
        FD_ZERO(&_fdRead_bak);
        _clients_change = true;
        _maxSock = SOCKET_ERROR;
        //memset(_szRecv, 0, sizeof(_szRecv));
    }

    ~CellServer()
    {
        Close();
        _sock = INVALID_SOCKET;
    }

    void setEventObj(INetEvent* event)
    {
        _pNetEvent = event;
    }

    void Start()
    {
        //std::mem_fn(&CellServer::OnRun)
        _thread = std::thread(&CellServer::OnRun, this);
        _taskServer.Start();
    }

    //处理网络消息
    void OnRun()
    {
        _clients_change = true;
        while (isRun())
        {
            //从缓冲区队列里取出客户数据
            if (!_clientsBuff.empty())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto pClient : _clientsBuff)
                {
                    _clients[pClient->sockfd()] = pClient;
                }
                _clientsBuff.clear();
                _clients_change = true;
            }

            //如果没有需要处理的客户端则跳过
            if (_clients.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }

            //伯克利套接字 BSD socket
            fd_set fdRead; //select监视的可读文件句柄集合
            ///fd_set fdWrite; //select监视的可写文件句柄集合
            //fd_set fdExp; //select监视的异常文件句柄集合
            //将fd_set清零使集合中不含任何SOCKET
            FD_ZERO(&fdRead);
            //FD_ZERO(&fdWrite);
            //FD_ZERO(&fdExp);
            if (_clients_change)
            {
                _clients_change = false;
                //将SOCKET加入fd_set集合
                _maxSock = _clients.begin()->second->sockfd();
                for (auto iter : _clients)
                {
                    FD_SET(iter.second->sockfd(), &fdRead);
                    if (_maxSock < iter.second->sockfd())
                    {
                        _maxSock = iter.second->sockfd();
                    }
                }
                memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
            }
            else
            {
                memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
            }

            //int nfds 集合中所有文件描述符的范围，而不是数量
            //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
            //timeout 本次select()的超时结束时间
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
            if (ret < 0)
            {
                printf("select任务结束。\n");
                Close();
                return;
            }
            else if (ret == 0)
            {
                continue;
            }
#ifdef _WIN32
            for (int n = 0; n < (int)fdRead.fd_count; n++)
            {
                auto iter = _clients.find(fdRead.fd_array[n]);
                if (iter != _clients.end())
                {
                    if (-1 == RecvData(iter->second)) //与服务器断开连接
                    {
                        if (_pNetEvent)
                        {
                            _pNetEvent->OnNetLeave(iter->second);
                        }
                        _clients_change = true;
                        _clients.erase(iter->first);
                    }
                }
                else
                {
                    printf("error. if (iter != _clients.end())\n");
                }
            }
#else
            std::vector<CellClientPtr> temp;
            for (auto iter : _clients)
            {
                if (FD_ISSET(iter.second->sockfd(), &fdRead))
                {
                    if (-1 == RecvData(iter.second)) //与服务器断开连接
                    {
                        if (_pNetEvent)
                        {
                            _pNetEvent->OnNetLeave(iter.second);
                        }
                        _clients_change = true;
                        temp.push_back(iter.second);
                    }
                }
            }
            for (auto pClient : temp)
            {
                _clients.erase(pClient->sockfd());
                delete pClient;
            }
#endif //_WIN32
        }
    }

    //接收数据 处理粘包 拆分包
    int RecvData(CellClientPtr pClient)
    {
        //接收客户端数据
        char* szRecv = pClient->msgBuf() + pClient->getLastPos();
        int nlen = (int)recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE - pClient->getLastPos(), 0);
        _pNetEvent->OnNetRecv(pClient);
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            //printf("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
            return -1;
        }
        //将收取收取到的数据拷贝到消息缓冲区
        //memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nlen);
        //消息缓冲区的数据尾部位置后移
        pClient->setLastPos(pClient->getLastPos() + nlen);
        //判断消息缓冲区的数据长度大于消息头DataHeader长度
        while (pClient->getLastPos() >= sizeof(DataHeader))
        {
            //这时就可以知道当前消息的长度
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            //判断消息缓冲区的数据长度大于消息长度
            if (pClient->getLastPos() >= header->dataLength)
            {
                //剩余未处理消息缓冲区消息的长度
                int nSize = pClient->getLastPos() - header->dataLength;
                //处理网络消息
                OnNetMsg(pClient, (DataHeaderPtr&)header);
                //将消息缓冲区剩余未处理数据前移
                memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
                pClient->setLastPos(nSize);
            }
            else
            {
                //消息缓冲区剩余数据不够一条完整消息
                break;
            }
        }
        return 0;
    }

    //响应网络消息
    virtual void OnNetMsg(CellClientPtr& pClient, DataHeaderPtr& header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    //关闭Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            for (auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
            }
            //关闭套接字closesocket
            closesocket(_sock);
#else
            for (auto iter : _clients)
            {
                close(iter.second->sockfd());
            }
            close(_sock);
#endif
            _clients.clear();
        }
    }

    //是否工作中
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }

    void addClient(CellClientPtr pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        //_mutex.lock();
        _clientsBuff.push_back(pClient);
        //_mutex.unlock();
    }

    size_t getClientCount()
    {
        return _clients.size() + _clientsBuff.size();
    }

    void addSendTask(CellClientPtr pClient, DataHeaderPtr header)
    {
        CellS2CTaskPtr task = std::make_shared<CellS2CTask>(pClient, header);
        _taskServer.addTask(task);
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::map<SOCKET, CellClientPtr> _clients;
    //缓冲客户队列
    std::vector<CellClientPtr> _clientsBuff;
    //缓冲队列的锁
    std::mutex _mutex;
    std::thread _thread;
    //网络事件对象
    INetEvent* _pNetEvent;
    //备份客户socket fd_set
    fd_set _fdRead_bak;
    //客户列表是否有变化
    bool _clients_change;
    SOCKET _maxSock;
    //第二缓冲区，双缓冲
    //char _szRecv[RECV_BUFF_SIZE];
    CellTaskServer _taskServer;
};

#endif // !_CELL_SERVER_HPP_
