#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "CELL.hpp"
#include "INetEvent.hpp"
#include "CELLClient.hpp"
#include "CELLSemaphore.hpp"

#include <vector>
#include <map>

//网络消息接收处理服务类
class CELLServer
{
public:
    CELLServer(int id)
    {
        _pNetEvent = nullptr;
        FD_ZERO(&_fdRead_bak);
        _clients_change = true;
        _maxSock = SOCKET_ERROR;
        //memset(_szRecv, 0, sizeof(_szRecv));
        _oldTime = CELLTime::getNowInMilliSec();
        _id = id;
        _taskServer.serverId = id;
    }

    ~CELLServer()
    {
        printf("CELLServer%d.~CELLServer exit begin\n", _id);
        Close();
        printf("CELLServer%d.~CELLServer exit end\n", _id);
    }

    void setEventObj(INetEvent* event)
    {
        _pNetEvent = event;
    }

    void Start()
    {
        _taskServer.Start();
        _thread.Start(
            //onCreat
            nullptr,
            //onRun
            [this](CELLThread* pThread) {
            OnRun(pThread);
            }, 
            //onClose
            [this](CELLThread* pThread) {
                ClearClients();
            }
        );
    }

    //处理网络消息
    void OnRun(CELLThread* pThread)
    {
        printf("CELLServer%d.OnRun begin\n", _id);
        while (pThread->isRun())
        {
            //从缓冲区队列里取出客户数据
            if (!_clientsBuff.empty())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto pClient : _clientsBuff)
                {
                    _clients[pClient->sockfd()] = pClient;
                    pClient->serverId = _id;
                    if (_pNetEvent)
                    {
                        _pNetEvent->OnNetJoin(pClient);;
                    }
                }
                _clientsBuff.clear();
                _clients_change = true;
            }

            //如果没有需要处理的客户端则跳过
            if (_clients.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                _oldTime = CELLTime::getNowInMilliSec();
                continue;
            }

            //伯克利套接字 BSD socket
            fd_set fdRead; //select监视的可读文件句柄集合
            fd_set fdWrite; //select监视的可写文件句柄集合
            //fd_set fdExp; //select监视的异常文件句柄集合
            
            if (_clients_change)
            {
                _clients_change = false;
                //将fd_set清零使集合中不含任何SOCKET
                FD_ZERO(&fdRead);
                //FD_ZERO(&fdWrite);
                //FD_ZERO(&fdExp);
                //将SOCKET加入fd_set集合
                _maxSock = _clients.begin()->first;
                for (auto client : _clients)
                {
                    FD_SET(client.first, &fdRead);
                    if (_maxSock < client.first)
                    {
                        _maxSock = client.first;
                    }
                }
                memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
            }
            else
            {
                memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
            }

            memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
            //memcpy(&fdExp, &_fdRead_bak, sizeof(fd_set));

            //int nfds 集合中所有文件描述符的范围，而不是数量
            //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
            //timeout 本次select()的超时结束时间
            timeval t = { 0,1 }; //s,us
            int ret = select((int)_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
            if (ret < 0)
            {
                printf("CELLServer%d.OnRun.select Error exit\n", _id);
                pThread->Exit();
                break;
            }
            ReadData(fdRead);
            WriteData(fdWrite);
            //WriteData(fdExp);
            CheckTime();
        }
        printf("CELLServer%d.OnRun exit\n", _id);
    }

    void CheckTime()
    {
        time_t nowTime = CELLTime::getNowInMilliSec();
        time_t dt = nowTime - _oldTime;
        _oldTime = nowTime;

        for (auto iter = _clients.begin(); iter != _clients.end();/**/)
        {
            //心跳检测
            if (iter->second->checkHeart(dt))
            {
                if (_pNetEvent)
                {
                    _pNetEvent->OnNetLeave(iter->second);
                }
                _clients_change = true;
                //delete iter->second;
                auto iterOld = iter++;
                _clients.erase(iterOld);
                continue;
            }

            //定时发送检测
            //iter->second->checkSend(dt);

            iter++;
        }
    }

    void WriteData(fd_set& fdWrite)
    {
#ifdef _WIN32
        for (int n = 0; n < (int)fdWrite.fd_count; n++)
        {
            auto iter = _clients.find(fdWrite.fd_array[n]);
            if (iter != _clients.end())
            {
                if (-1 == iter->second->SendDataReal()) //与服务器断开连接
                {
                    if (_pNetEvent)
                    {
                        _pNetEvent->OnNetLeave(iter->second);
                    }
                    _clients_change = true;
                    //delete iter->second;
                    _clients.erase(iter);
                }
            }
        }
#else
        std::vector<CELLClientPtr> temp;
        for (auto iter : _clients)
        {
            if (FD_ISSET(iter.second->sockfd(), &fdWrite))
            {
                if (-1 == iter->second->SendDataReal()) //与服务器断开连接
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

    void ReadData(fd_set& fdRead)
    {
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
                    //delete iter->second;
                    _clients.erase(iter);
                }
            }
            else
            {
                printf("error. if (iter != _clients.end())\n");
            }
        }
#else
        std::vector<CELLClientPtr> temp;
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

    //接收数据 处理粘包 拆分包
    int RecvData(CELLClientPtr pClient)
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
                OnNetMsg(pClient, header);
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
    virtual void OnNetMsg(CELLClientPtr& pClient, DataHeader* header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    //关闭Socket
    void Close()
    {
        printf("CELLServer%d.Close begin\n", _id);
        _taskServer.Close();
        _thread.Close();
        printf("CELLServer%d.Close end\n", _id);
    }

    void addClient(CELLClientPtr pClient)
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

    void addSendTask(CELLClientPtr pClient, DataHeader* header)
    {
        //CELLS2CTaskPtr task = std::make_shared<CELLS2CTask>(pClient, header);
        _taskServer.addTask([pClient, header]() {
            pClient->SendData(header);
        });
    }

private:
    void ClearClients()
    {
        /*for (auto iter : _clients)
        {
            delete iter.second;
        }*/
        _clients.clear();
        _clientsBuff.clear();
    }

private:
    //正式客户队列
    std::map<SOCKET, CELLClientPtr> _clients;
    //缓冲客户队列
    std::vector<CELLClientPtr> _clientsBuff;
    //网络事件对象
    INetEvent* _pNetEvent;
    //
    CELLTaskServer _taskServer;
    //缓冲队列的锁
    std::mutex _mutex;
    //备份客户socket fd_set
    fd_set _fdRead_bak;
    CELLThread _thread;
    //
    int _id = -1;
    //客户列表是否有变化
    bool _clients_change;
    SOCKET _maxSock;
    //旧的时间戳
    time_t _oldTime;
};

#endif // !_CELL_SERVER_HPP_
