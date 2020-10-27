#ifndef _EASY_TCP_SERVER_HPP_
#define _EASY_TCP_SERVER_HPP_

#include "CELL.hpp"
#include "CELLClient.hpp"
#include "CELLServer.hpp"
#include "INetEvent.hpp"
#include "CELLNetWork.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class EasyTcpServer :public INetEvent
{
private:
    CELLThread _thread;
    //消息处理对象，内部会创建线程
    std::vector<CELLServerPtr> _cellServers;
    //每秒消息计时
    CELLTimestamp _tTime;
    SOCKET _sock;
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
        CELLNetWork::Init();
        if (INVALID_SOCKET != _sock)
        {
            CELLLog::Info("<socket=%d>关闭旧连接...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            CELLLog::Info("<socket=%d>错误，建立Socket失败...\n", (int)_sock);
        }
        else
        {
            CELLLog::Info("建立<socket=%d>成功...\n", (int)_sock);
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
            CELLLog::Info("错误，绑定网络端口<%d>失败...\n", port);
        }
        else
        {
            CELLLog::Info("绑定网络端口<%d>成功...\n", port);
        }
        return ret;
    }

    //监听端口号
    int Listen(int n) //等待连接队列的最大长度
    {
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            CELLLog::Info("<socket=%d>错误，监听网络端口失败\n", (int)_sock);
        }
        else
        {
            CELLLog::Info("<socket=%d>监听网络端口成功...\n", (int)_sock);
        }
        return ret;
    }

    void Start(int nCELLServer)
    {
        for (int n = 0; n < nCELLServer; n++)
        {
            CELLServerPtr ser = std::make_shared<CELLServer>(n + 1);
            _cellServers.push_back(ser);
            //注册网络事件接受对象
            ser->setEventObj(this);
            //启动消息处理线程
            ser->Start();
        }
        _thread.Start(
            //onCreat
            nullptr,
            //onRun
            [this](CELLThread* pThread) {
                OnRun(pThread);
            },
            //onClose
            nullptr
            );
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
            CELLLog::Info("<socket=%d>错误，接受到无效客户端SOCKET...\n", (int)_sock);
        }
        else
        {
            //将新客户端分配给客户数量最少的CELLServer
            CELLClientPtr task(new CELLClient(cSock));
            addClientToCELLServer(task);
            //addClientToCELLServer(std::make_shared<CELLClient>(cSock));
            //获取IP地址 inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToCELLServer(CELLClientPtr pClient)
    {
        //查找客户数量最少的CELLServers消息处理线程
        auto pMinServer = _cellServers[0];
        for (auto pCELLServer : _cellServers)
        {
            if (pMinServer->getClientCount() > pCELLServer->getClientCount())
            {
                pMinServer = pCELLServer;
            }
        }
        pMinServer->addClient(pClient);
        
    }

    //关闭Socket
    void Close()
    {
        CELLLog::Info("EasyTcpServer.Close begin\n");
        _thread.Close();
        if (INVALID_SOCKET != _sock)
        {
            _cellServers.clear();
            //关闭套接字
#ifdef _WIN32
            closesocket(_sock);
#else
            close(_sock);
#endif
            _sock = INVALID_SOCKET;
        }
        CELLLog::Info("EasyTcpServer.Close end\n");
    }

    //只会被一个线程调用
    virtual void OnNetJoin(CELLClientPtr& pClient)
    {
        _clientCount++;
        //CELLLog::Info("client<%d> join\n", pClient->sockfd());
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetLeave(CELLClientPtr& pClient)
    {
        _clientCount--;
        //CELLLog::Info("client<%d> leave\n", pClient->sockfd());
    }

    //cellServer 多线程调用 不安全
    virtual void OnNetMsg(CELLServer* pCELLServer, CELLClientPtr& pClient, DataHeader* header)
    {
        _msgCount++;
    }

    virtual void OnNetRecv(CELLClientPtr& pClient)
    {
        _recvCount++;
    }

private:
    //处理网络消息
    void OnRun(CELLThread* pThread)
    {
        while (pThread->isRun())
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
            timeval t = { 0, 10 }; //s,us
            int ret = select((int)_sock + 1, &fdRead, 0, 0, &t);
            if (ret < 0)
            {
                CELLLog::Info("EasyTcpServer.OnRun Accept select exit.\n");
                pThread->Exit();
                break;
            }
            //判断描述符(socket)是否在集合中
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
            }
        }
    }

    //计算并输出每秒收到的网络消息
    void time4msg()
    {
        auto t1 = _tTime.getElapseSecond();
        if (t1 >= 1.0)
        {
            CELLLog::Info("thread<%zd>, time<%f>, socket<%d>, clients<%d>, recvCount<%d>, msgCount<%d>\n", _cellServers.size(), t1, (int)_sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
            _recvCount = 0;
            _msgCount = 0;
            _tTime.update();
        }
    }

};

#endif // !_EASY_TCP_SERVER_HPP_
