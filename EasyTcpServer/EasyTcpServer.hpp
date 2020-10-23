#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
    #define FD_SETSIZE    10000
    #define WIN32_LEAN_AND_MEAN //��ֹwindows.h��WinSock2.h���ض���
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa��������ʱ������������
    #define _CRT_SECURE_NO_WARNINGS //scanf������strcpy����
    //#define SOCKET int
    #include <windows.h>
    #include <WinSock2.h>
    //��̬���ӿ⣬����WSAStartup��WSACleanup
    #pragma comment(lib,"ws2_32.lib")
#else
    #include <unistd.h> //unix std������windows.h
    #include <arpa/inet.h> //����WinSock2.h
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

//��������С��Ԫ��С
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

//�ͻ�����������
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

    //��������
    int SendData(DataHeaderPtr header)
    {
        int ret = SOCKET_ERROR;
        //Ҫ���͵����ݳ���
        int nSendLen = header->dataLength;
        //Ҫ���͵�����
        const char* pSendData = (const char*)header.get();
        while (true)
        {
            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
            {
                //����ɿ��������ݳ���
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //��������
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //����ʣ������λ��
                pSendData += nCopyLen;
                //����ʣ�����ݳ���
                nSendLen -= nCopyLen;
                //��������
                ret = (int)send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
                //����β��λ������
                _lastSendPos = 0;
                //���ʹ���
                if (SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else
            {
                //��Ҫ���͵����ݿ��������ͻ�����β��
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                //��������β��λ��
                _lastSendPos += nSendLen;
                break;
            }
        }   
        return ret;
    }

private:
    //fd_set file desc set
    SOCKET _sockfd;
    //�ڶ������� ��Ϣ������
    char _szMsgBuf[RECV_BUFF_SIZE];
    //��Ϣ������������β��λ��
    int _lastPos;
    //�ڶ������� ���ͻ�����
    char _szSendBuf[SEND_BUFF_SIZE];
    //���ͻ�����������β��λ��
    int _lastSendPos;
};

//�����¼��ӿ�
class INetEvent
{
public:
    //�ͻ��˼����¼�
    virtual void OnNetJoin(ClientSocketPtr& pClient) = 0;
    //�ͻ����뿪�¼�
    virtual void OnNetLeave(ClientSocketPtr& pClient) = 0;
    //�ͻ�����Ϣ�¼�
    virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header) = 0;
    //recv�¼�
    virtual void OnNetRecv(ClientSocketPtr& pClient) = 0;
};

//������Ϣ��������
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

    //ִ������
    void doTask()
    {
        _pClient->SendData(_pHeader);
    }

};

//������Ϣ���մ��������
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

    //����������Ϣ
    void OnRun()
    {
        _clients_change = true;
        while (isRun())
        {
            //�ӻ�����������ȡ���ͻ�����
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

            //���û����Ҫ����Ŀͻ���������
            if (_clients.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }

            //�������׽��� BSD socket
            fd_set fdRead; //select���ӵĿɶ��ļ��������
            ///fd_set fdWrite; //select���ӵĿ�д�ļ��������
            //fd_set fdExp; //select���ӵ��쳣�ļ��������
            //��fd_set����ʹ�����в����κ�SOCKET
            FD_ZERO(&fdRead);
            //FD_ZERO(&fdWrite);
            //FD_ZERO(&fdExp);
            if (_clients_change)
            {
                _clients_change = false;
                //��SOCKET����fd_set����
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
            
            //int nfds �����������ļ��������ķ�Χ������������
            //�������ļ������������ֵ��1����windows�������������ν������д0
            //timeout ����select()�ĳ�ʱ����ʱ��
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
            if (ret < 0)
            {
                printf("select���������\n");
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
                    if (-1 == RecvData(iter->second)) //��������Ͽ�����
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
                    if (-1 == RecvData(iter.second)) //��������Ͽ�����
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

    //�������� ����ճ�� ��ְ�
    int RecvData(ClientSocketPtr pClient)
    {
        //���տͻ�������
        char* szRecv = pClient->msgBuf() + pClient->getLastPos();
        int nlen = (int)recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE - pClient->getLastPos(), 0);
        _pNetEvent->OnNetRecv(pClient);
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            //printf("�ͻ���<Socket=%d>���˳������������\n", pClient->sockfd());
            return -1;
        }
        //����ȡ��ȡ�������ݿ�������Ϣ������
        //memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nlen);
        //��Ϣ������������β��λ�ú���
        pClient->setLastPos(pClient->getLastPos() + nlen);
        //�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
        while (pClient->getLastPos() >= sizeof(DataHeader))
        {
            //��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            //�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
            if (pClient->getLastPos() >= header->dataLength)
            {
                //ʣ��δ������Ϣ��������Ϣ�ĳ���
                int nSize = pClient->getLastPos() - header->dataLength;
                //����������Ϣ
                OnNetMsg(pClient, header);
                //����Ϣ������ʣ��δ��������ǰ��
                memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
                pClient->setLastPos(nSize);
            }
            else
            {
                //��Ϣ������ʣ�����ݲ���һ��������Ϣ
                break;
            }
        }
        return 0;
    }

    //��Ӧ������Ϣ
    virtual void OnNetMsg(ClientSocketPtr pClient, DataHeader* header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
            for (auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
            }
#ifdef _WIN32
            //�ر��׽���closesocket
            closesocket(_sock);
#else
            close(_sock);
#endif
            _clients.clear();
        }
    }

    //�Ƿ�����
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
    //��ʽ�ͻ�����
    std::map<SOCKET, ClientSocketPtr> _clients;
    //����ͻ�����
    std::vector<ClientSocketPtr> _clientsBuff;
    //������е���
    std::mutex _mutex;
    std::thread _thread;
    //�����¼�����
    INetEvent* _pNetEvent;
    //���ݿͻ�socket fd_set
    fd_set _fdRead_bak;
    //�ͻ��б��Ƿ��б仯
    bool _clients_change;
    SOCKET _maxSock;
    //�ڶ���������˫����
    //char _szRecv[RECV_BUFF_SIZE];
    CellTaskServer _taskServer;
};

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
            timeval t = { 0,10 }; //s,ms
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
            addClientToCellServer(std::make_shared<ClientSocket>(cSock));
            //��ȡIP��ַ inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToCellServer(ClientSocketPtr pClient)
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
    virtual void OnNetJoin(ClientSocketPtr& pClient)
    {
        _clientCount++;
        //printf("client<%d> join\n", pClient->sockfd());
    }

    //cellServer ���̵߳��� ����ȫ
    virtual void OnNetLeave(ClientSocketPtr& pClient)
    {
        _clientCount--;
        //printf("client<%d> leave\n", pClient->sockfd());
    }

    //cellServer ���̵߳��� ����ȫ
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
