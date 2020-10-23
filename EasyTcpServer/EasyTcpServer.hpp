#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#include "CELL.hpp"
#include "CELLClient.hpp"
#include "CELLServer.hpp"
#include "INetEvent.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class EasyTcpServer :public INetEvent
{
private:
    SOCKET _sock;
    //��Ϣ��������ڲ��ᴴ���߳�
    std::vector<CellServerPtr> _cellServers;
    //ÿ����Ϣ��ʱ
    CELLTimestamp _tTime;
protected:
    //SOCKET recv����
    std::atomic_int _recvCount;
    //�յ���Ϣ����
    std::atomic_int _msgCount;
    //�ͻ��˼���
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

    //��ʼ��Socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        //����Windows socket 2.x����
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        if (isRun())
        {
            printf("<socket=%d>�رվ�����...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("<socket=%d>���󣬽���Socketʧ��...\n", (int)_sock);
        }
        else
        {
            printf("����<socket=%d>�ɹ�...\n", (int)_sock);
        }
        return _sock;
    }

    //��IP�Ͷ˿ں�
    int Bind(const char* ip, unsigned short port)
    {
        if (INVALID_SOCKET == _sock)
         {
            InitSocket();
         }
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET; //��ַ��(IPV4)
        _sin.sin_port = htons(port); //�˿ںţ�host to net unsigned short

#ifdef _WIN32
        if (ip)
        {
            _sin.sin_addr.S_un.S_addr = inet_addr(ip); //������ַ
        }
        else
        {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
        }
#else
        if (ip)
        {
            _sin.sin_addr.s_addr = inet_addr(ip); //������ַ
        }
        else
        {
            _sin.sin_addr.s_addr = INADDR_ANY;
        }
#endif
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("���󣬰�����˿�<%d>ʧ��...\n", port);
        }
        else
        {
            printf("������˿�<%d>�ɹ�...\n", port);
        }
        return ret;
    }

    //�����˿ں�
    int Listen(int n) //�ȴ����Ӷ��е���󳤶�
    {
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>���󣬼�������˿�ʧ��\n", (int)_sock);
        }
        else
        {
            printf("<socket=%d>��������˿ڳɹ�...\n", (int)_sock);
        }
        return ret;
    }

    void Start(int nCellServer)
    {
        for (int n = 0; n < nCellServer; n++)
        {
            CellServerPtr ser = std::make_shared<CellServer>(_sock);
            _cellServers.push_back(ser);
            //ע�������¼����ܶ���
            ser->setEventObj(this);
            //������Ϣ�����߳�
            ser->Start();
        }
    }

    //����������Ϣ
    bool OnRun()
    {
        if (isRun())
        {
            time4msg();
            //�������׽��� BSD socket
            fd_set fdRead; //select���ӵĿɶ��ļ��������
            //fd_set fdWrite; //select���ӵĿ�д�ļ��������
            //fd_set fdExp; //select���ӵ��쳣�ļ��������
            //��fd_set����ʹ�����в����κ�SOCKET
            FD_ZERO(&fdRead);
            //FD_ZERO(&fdWrite);
            //FD_ZERO(&fdExp);
            //��SOCKET����fd_set����
            FD_SET(_sock, &fdRead);
            //FD_SET(_sock, &fdWrite);
            //FD_SET(_sock, &fdExp);
            //int nfds �����������ļ��������ķ�Χ������������
            //�������ļ������������ֵ��1����windows�������������ν������д0
            //timeout ����select()�ĳ�ʱ����ʱ��
            timeval t = { 0, 10 }; //s,ms
            int ret = select((int)_sock + 1, &fdRead, 0, 0, &t);
            if (ret < 0)
            {
                printf("Accept Select���������\n");
                Close();
                return false;
            }
            //�ж�������(socket)�Ƿ��ڼ�����
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
            }
            return true;
        }
        return false;
    }

    //���㲢���ÿ���յ���������Ϣ
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

    //���ܿͻ�������
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
            printf("<socket=%d>���󣬽��ܵ���Ч�ͻ���SOCKET...\n", (int)_sock);
        }
        else
        {
            //���¿ͻ��˷�����ͻ��������ٵ�CellServer
            CellClientPtr task(new CellClient(cSock));
            addClientToCellServer(task);
            //addClientToCellServer(std::make_shared<CellClient>(cSock));
            //��ȡIP��ַ inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToCellServer(CellClientPtr pClient)
    {
        //���ҿͻ��������ٵ�CellServers��Ϣ�����߳�
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

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            //�ر��׽���closesocket
            closesocket(_sock);
            //���Windows socket����
            WSACleanup();
#else
            close(_sock);
#endif
        }
    }

    //�Ƿ�����
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }

    //ֻ�ᱻһ���̵߳���
    virtual void OnNetJoin(CellClientPtr& pClient)
    {
        _clientCount++;
        //printf("client<%d> join\n", pClient->sockfd());
    }

    //cellServer ���̵߳��� ����ȫ
    virtual void OnNetLeave(CellClientPtr& pClient)
    {
        _clientCount--;
        //printf("client<%d> leave\n", pClient->sockfd());
    }

    //cellServer ���̵߳��� ����ȫ
    virtual void OnNetMsg(CellServer* pCellServer, CellClientPtr& pClient, DataHeaderPtr& header)
    {
        _msgCount++;
    }

    virtual void OnNetRecv(CellClientPtr& pClient)
    {
        _recvCount++;
    }

};

#endif //_EasyTcpServer_hpp_
