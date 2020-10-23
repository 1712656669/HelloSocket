#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
    #define FD_SETSIZE    10000
    #define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
    #define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数
    //#define SOCKET int
    #include <windows.h>
    #include <WinSock2.h>
    //静态链接库，解析WSAStartup和WSACleanup
    #pragma comment(lib,"ws2_32.lib")
#else
    #include <unistd.h> //unix std，类似windows.h
    #include <arpa/inet.h> //类似WinSock2.h
    #include <string.h>

    #define SOCKET int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define SOCKET_ERROR            (-1)
#endif //_WIN32

#include <stdio.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"

//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif

class CellS2CTask;
class ClientSocket;
class CellServer;
typedef std::shared_ptr<CellS2CTask> CellS2CTaskPtr;
typedef std::shared_ptr<ClientSocket> ClientSocketPtr;
typedef std::shared_ptr<CellServer> CellServerPtr;
typedef std::shared_ptr<LoginResult> LoginResultPtr;
typedef std::shared_ptr<DataHeader> DataHeaderPtr;

//客户端数据类型
class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
        _lastPos = 0;
        memset(_szSendBuf, 0, SEND_BUFF_SIZE);
        _lastSendPos = 0;
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }

    void setLastPos(int pos)
    {
        _lastPos = pos;
    }

    //发送数据
    int SendData(DataHeaderPtr header)
    {
        int ret = SOCKET_ERROR;
        //要发送的数据长度
        int nSendLen = header->dataLength;
        //要发送的数据
        const char* pSendData = (const char*)header.get();
        while (true)
        {
            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
            {
                //计算可拷贝的数据长度
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //拷贝数据
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //计算剩余数据位置
                pSendData += nCopyLen;
                //计算剩余数据长度
                nSendLen -= nCopyLen;
                //发送数据
                ret = (int)send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
                //数据尾部位置清零
                _lastSendPos = 0;
                //发送错误
                if (SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else
            {
                //将要发送的数据拷贝到发送缓冲区尾部
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                //计算数据尾部位置
                _lastSendPos += nSendLen;
                break;
            }
        }   
        return ret;
    }

private:
    //fd_set file desc set
    SOCKET _sockfd;
    //第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE];
    //消息缓冲区的数据尾部位置
    int _lastPos;
    //第二缓冲区 发送缓冲区
    char _szSendBuf[SEND_BUFF_SIZE];
    //发送缓冲区的数据尾部位置
    int _lastSendPos;
};

//网络事件接口
class INetEvent
{
public:
    //客户端加入事件
    virtual void OnNetJoin(ClientSocketPtr& pClient) = 0;
    //客户端离开事件
    virtual void OnNetLeave(ClientSocketPtr& pClient) = 0;
    //客户端消息事件
    virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header) = 0;
    //recv事件
    virtual void OnNetRecv(ClientSocketPtr& pClient) = 0;
};

//网络消息发送任务
class CellS2CTask :public CellTask
{
private:
    ClientSocketPtr _pClient;
    DataHeaderPtr _pHeader;

public:
    CellS2CTask(ClientSocketPtr pClient, DataHeaderPtr pHeader)
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
            std::vector<ClientSocketPtr> temp;
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
    int RecvData(ClientSocketPtr pClient)
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
    virtual void OnNetMsg(ClientSocketPtr pClient, DataHeader* header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    //关闭Socket
    void Close()
    {
        if (isRun())
        {
            for (auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
            }
#ifdef _WIN32
            //关闭套接字closesocket
            closesocket(_sock);
#else
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

    void addClient(ClientSocketPtr pClient)
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

    void addSendTask(ClientSocketPtr pClient, DataHeaderPtr header)
    {
        
        CellS2CTaskPtr task = std::make_shared<CellS2CTask>(pClient, header);
        _taskServer.addTask(task);
    }

private:
    SOCKET _sock;
    //正式客户队列
    std::map<SOCKET, ClientSocketPtr> _clients;
    //缓冲客户队列
    std::vector<ClientSocketPtr> _clientsBuff;
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

class EasyTcpServer :public INetEvent
{
private:
    SOCKET _sock;
    //消息处理对象，内部会创建线程
    std::vector<CellServerPtr> _cellServers;
    //每秒消息计时
    CELLTimestamp _tTime;
protected:
    //SOCKET recv计数
    std::atomic_int _recvCount;
    //收到消息计数
    std::atomic_int _msgCount;
    //客户端计数
    std::atomic_int _clientCount;

public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
        _msgCount = 0;
        _clientCount = 0;
    }

    virtual ~EasyTcpServer()
    {
        Close();
    }

    //初始化Socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        //启动Windows socket 2.x环境
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        if (isRun())
        {
            printf("<socket=%d>关闭旧连接...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("<socket=%d>错误，建立Socket失败...\n", (int)_sock);
        }
        else
        {
            printf("建立<socket=%d>成功...\n", (int)_sock);
        }
        return _sock;
    }

    //绑定IP和端口号
    int Bind(const char* ip, unsigned short port)
    {
        if (INVALID_SOCKET == _sock)
         {
            InitSocket();
         }
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET; //地址族(IPV4)
        _sin.sin_port = htons(port); //端口号，host to net unsigned short

#ifdef _WIN32
        if (ip)
        {
            _sin.sin_addr.S_un.S_addr = inet_addr(ip); //本机地址
        }
        else
        {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
        }
#else
        if (ip)
        {
            _sin.sin_addr.s_addr = inet_addr(ip); //本机地址
        }
        else
        {
            _sin.sin_addr.s_addr = INADDR_ANY;
        }
#endif
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("错误，绑定网络端口<%d>失败...\n", port);
        }
        else
        {
            printf("绑定网络端口<%d>成功...\n", port);
        }
        return ret;
    }

    //监听端口号
    int Listen(int n) //等待连接队列的最大长度
    {
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>错误，监听网络端口失败\n", (int)_sock);
        }
        else
        {
            printf("<socket=%d>监听网络端口成功...\n", (int)_sock);
        }
        return ret;
    }

    void Start(int nCellServer)
    {
        for (int n = 0; n < nCellServer; n++)
        {
            CellServerPtr ser = std::make_shared<CellServer>(_sock);
            _cellServers.push_back(ser);
            //注册网络事件接受对象
            ser->setEventObj(this);
            //启动消息处理线程
            ser->Start();
        }
    }

    //处理网络消息
    bool OnRun()
    {
        if (isRun())
        {
            time4msg();
            //伯克利套接字 BSD socket
            fd_set fdRead; //select监视的可读文件句柄集合
            //fd_set fdWrite; //select监视的可写文件句柄集合
            //fd_set fdExp; //select监视的异常文件句柄集合
            //将fd_set清零使集合中不含任何SOCKET
            FD_ZERO(&fdRead);
            //FD_ZERO(&fdWrite);
            //FD_ZERO(&fdExp);
            //将SOCKET加入fd_set集合
            FD_SET(_sock, &fdRead);
            //FD_SET(_sock, &fdWrite);
            //FD_SET(_sock, &fdExp);
            //int nfds 集合中所有文件描述符的范围，而不是数量
            //即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
            //timeout 本次select()的超时结束时间
            timeval t = { 0,10 }; //s,ms
            int ret = select((int)_sock + 1, &fdRead, 0, 0, &t);
            if (ret < 0)
            {
                printf("Accept Select任务结束。\n");
                Close();
                return false;
            }
            //判断描述符(socket)是否在集合中
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
            }
            return true;
        }
        return false;
    }

    //计算并输出每秒收到的网络消息
    void time4msg()
    {
        auto t1 = _tTime.getElapseSecond();
        if (t1 >= 1.0)
        {
            printf("thread<%zd>, time<%f>, socket<%d>, clients<%d>, recvCount<%d>, msgCount<%d>\n", _cellServers.size(), t1, (int)_sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
            _recvCount = 0;
            _msgCount = 0;
            _tTime.update();
        }
    }

    //接受客户端连接
    SOCKET Accept()
    {
        sockaddr_in clientAddr = {};
        int nAddrLen = sizeof(sockaddr_in);
        SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
        cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if (INVALID_SOCKET == cSock)
        {
            printf("<socket=%d>错误，接受到无效客户端SOCKET...\n", (int)_sock);
        }
        else
        {
            //将新客户端分配给客户数量最少的CellServer
            addClientToCellServer(std::make_shared<ClientSocket>(cSock));
            //获取IP地址 inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToCellServer(ClientSocketPtr pClient)
    {
        //查找客户数量最少的CellServers消息处理线程
        auto pMinServer = _cellServers[0];
        for (auto pCellServer : _cellServers)
        {
            if (pMinServer->getClientCount() > pCellServer->getClientCount())
            {
                pMinServer = pCellServer;
            }
        }
        pMinServer->addClient(pClient);
        OnNetJoin(pClient);
    }

    //关闭Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            //关闭套接字closesocket
            closesocket(_sock);
            //清除Windows socket环境
            WSACleanup();
#else
            close(_sock);
#endif
        }
    }

    //是否工作中
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }

    //只会被一个线程调用
    virtual void OnNetJoin(ClientSocketPtr& pClient)
    {
        _clientCount++;
        //printf("client<%d> join\n", pClient->sockfd());
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetLeave(ClientSocketPtr& pClient)
    {
        _clientCount--;
        //printf("client<%d> leave\n", pClient->sockfd());
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header)
    {
        _msgCount++;
    }

    virtual void OnNetRecv(ClientSocketPtr& pClient)
    {
        _recvCount++;
    }

};

#endif //_EasyTcpServer_hpp_
