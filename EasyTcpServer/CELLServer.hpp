#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "CELL.hpp"
#include "INetEvent.hpp"
#include "CELLClient.hpp"

#include <vector>
#include <map>

//������Ϣ��������
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
            std::vector<CellClientPtr> temp;
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
    int RecvData(CellClientPtr pClient)
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
                OnNetMsg(pClient, (DataHeaderPtr&)header);
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
    virtual void OnNetMsg(CellClientPtr& pClient, DataHeaderPtr& header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            for (auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
            }
            //�ر��׽���closesocket
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

    //�Ƿ�����
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
    //��ʽ�ͻ�����
    std::map<SOCKET, CellClientPtr> _clients;
    //����ͻ�����
    std::vector<CellClientPtr> _clientsBuff;
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

#endif // !_CELL_SERVER_HPP_
