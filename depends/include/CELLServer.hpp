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
        _fdRead = new fd_set;
        _fdWrite = new fd_set;
        _fdRead_bak = new fd_set;
        FD_ZERO(_fdRead_bak);
        _clients_change = true;
        _maxSock = SOCKET_ERROR;
        //memset(_szRecv, 0, sizeof(_szRecv));
        _oldTime = CELLTime::getNowInMilliSec();
        _id = id;
        _taskServer.serverId = id;
    }

    ~CELLServer()
    {
        CELLLog::Info("CELLServer%d.~CELLServer exit begin\n", _id);
        Close();
        CELLLog::Info("CELLServer%d.~CELLServer exit end\n", _id);
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
        CELLLog::Info("CELLServer%d.OnRun begin\n", _id);
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
            
            if (_clients_change)
            {
                _clients_change = false;
                //将fd_set清零使集合中不含任何SOCKET
                FD_ZERO(_fdRead);
                FD_ZERO(_fdWrite);
                //FD_ZERO(fdExp);
                _maxSock = _clients.begin()->first;
                for (auto client : _clients)
                {
                    //将SOCKET加入fd_set集合
                    FD_SET(client.first, _fdRead);
                    if (_maxSock < client.first)
                    {
                        _maxSock = client.first;
                    }
                }
                memcpy(_fdRead_bak, _fdRead, sizeof(fd_set));
            }
            else
            {
                memcpy(_fdRead, _fdRead_bak, sizeof(fd_set));
            }

            memcpy(_fdWrite, _fdRead_bak, sizeof(fd_set));
            //memcpy(fdExp, _fdRead_bak, sizeof(fd_set));

            //int nfds 集合中所有文件描述符的范围，而不是数量
            //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
            //timeout 本次select()的超时结束时间
            timeval t = { 0,1 }; //s,us
            int ret = select((int)_maxSock + 1, _fdRead, _fdWrite, nullptr, &t);
            if (ret < 0)
            {
                CELLLog::Info("CELLServer%d.OnRun.select Error exit\n", _id);
                pThread->Exit();
                break;
            }
            ReadData(_fdRead);
            WriteData(_fdWrite);
            //WriteData(fdExp);
            CheckTime();
        }
        CELLLog::Info("CELLServer%d.OnRun exit\n", _id);
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

    void WriteData(fd_set* _fdWrite)
    {
#ifdef _WIN32
        for (int n = 0; n < (int)_fdWrite->fd_count; n++)
        {
            auto iter = _clients.find(_fdWrite->fd_array[n]);
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
            if (FD_ISSET(iter.second->sockfd(), _fdWrite))
            {
                if (-1 == iter.second->SendDataReal()) //与服务器断开连接
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
        }
#endif //_WIN32
    }

    void ReadData(fd_set* _fdRead)
    {
#ifdef _WIN32
        for (int n = 0; n < (int)_fdRead->fd_count; n++)
        {
            auto iter = _clients.find(_fdRead->fd_array[n]);
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
                CELLLog::Info("error. iter == _clients.end()\n");
            }
        }
#else
        std::vector<CELLClientPtr> temp;
        for (auto iter : _clients)
        {
            if (FD_ISSET(iter.first, _fdRead))
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
        }
#endif //_WIN32
    }

    //接收数据 处理粘包 拆分包
    int RecvData(CELLClientPtr pClient)
    {
        //接收客户端数据
        int nlen = pClient->RecvData();
        if (nlen <= 0)
        {
            //CELLLog::Info("客户端<Socket=%d>已退出，任务结束。\n", pClient->sockfd());
            return -1;
        }
        //触发<接收到网络数据>事件
        _pNetEvent->OnNetRecv(pClient);

        //循环 判断数据是否有消息需要处理
        while (pClient->hasMsg())
        {
            //处理网络消息
            OnNetMsg(pClient, pClient->front_msg());
            //移除消息队列（缓冲区）最前的一条数据
            pClient->pop_front_msg();
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
        CELLLog::Info("CELLServer%d.Close begin\n", _id);
        _taskServer.Close();
        _thread.Close();
        CELLLog::Info("CELLServer%d.Close end\n", _id);
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
    fd_set* _fdRead_bak;
    CELLThread _thread;
    //
    int _id = -1;
    //客户列表是否有变化
    bool _clients_change;
    SOCKET _maxSock;
    //旧的时间戳
    time_t _oldTime;
    //伯克利套接字 BSD socket
    fd_set* _fdRead; //select监视的可读文件句柄集合
    fd_set* _fdWrite; //select监视的可写文件句柄集合
};

#endif // !_CELL_SERVER_HPP_
