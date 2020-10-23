#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
    #define FD_SETSIZE    10000
    #define WIN32_LEAN_AND_MEAN //��ֹwindows.h��WinSock2.h���ض���
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa��������ʱ������������
    #define _CRT_SECURE_NO_WARNINGS //scanf������strcpy����
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
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

//��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif

class ClientSocket
{
public:
    ClientSocket(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
        _lastPos = 0;
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

private:
    SOCKET _sockfd; //fd_set file desc set
    //�ڶ������� ��Ϣ������
    char _szMsgBuf[RECV_BUFF_SIZE * 10];
    //��Ϣ������������β��λ��
    int _lastPos;
};

class EasyTcpServer
{
private:
    SOCKET _sock;
    std::vector<ClientSocket*> _clients;
    CELLTimestamp _tTime;
    int _recvCount;

public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
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
        _sin.sin_port = htons(4567); //�˿ںţ�host to net unsigned short

#ifdef _WIN32
        if (ip)
        {
            _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //������ַ
        }
        else
        {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
        }
#else
        if (ip)
        {
            _sin.sin_addr.s_addr = inet_addr("127.0.0.1"); //������ַ
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
            //NewUserJoin userJoin;
            //SendDataToAll(&userJoin);
            _clients.push_back(new ClientSocket(cSock));
            printf("<socket=%d>�¿ͻ�<%d>���룺socket = %d��IP = %s\n", (int)_sock, (int)_clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
        }
        return cSock;
    }

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                closesocket(_clients[n]->sockfd());
                delete _clients[n];
            }
            //�ر��׽���closesocket
            closesocket(_sock);
            //���Windows socket����
            WSACleanup();
#else
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                close(_clients[n]->sockfd());
                delete _clients[n];
            }
            close(_sock);
#endif
            _clients.clear();
        }
    }

    //����������Ϣ
    int _nCount = 0;
    bool OnRun()
    {
        if (isRun())
        {
            //�������׽��� BSD socket
            fd_set fdRead; //select���ӵĿɶ��ļ��������
            fd_set fdWrite; //select���ӵĿ�д�ļ��������
            fd_set fdExp; //select���ӵ��쳣�ļ��������
            //��fd_set����ʹ�����в����κ�SOCKET
            FD_ZERO(&fdRead);
            FD_ZERO(&fdWrite);
            FD_ZERO(&fdExp);
            //��SOCKET����fd_set����
            FD_SET(_sock, &fdRead);
            FD_SET(_sock, &fdWrite);
            FD_SET(_sock, &fdExp);

            SOCKET maxSock = _sock;
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                FD_SET(_clients[n]->sockfd(), &fdRead);
                if (maxSock < _clients[n]->sockfd())
                {
                    maxSock = _clients[n]->sockfd();
                }
            }
            //int nfds �����������ļ��������ķ�Χ������������
            //�������ļ������������ֵ��1����windows�������������ν������д0
            //timeout ����select()�ĳ�ʱ����ʱ��
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
            //printf("select ret=%d count=%d\n", ret, _nCount++);
            if (ret < 0)
            {
                printf("select���������\n");
                Close();
                return false;
            }
            //�ж�������(socket)�Ƿ��ڼ�����
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
                //return true;
            }
            for (int n = (int)_clients.size() - 1; n >= 0; n--)
            {
                if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
                {
                    if (-1 == RecvData(_clients[n]))
                    {
                        //std::vector<SOCKET>::iterator
                        auto iter = _clients.begin() + n;
                        if (iter != _clients.end())
                        {
                            delete _clients[n];
                            _clients.erase(iter);
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    //�Ƿ�����
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }

    //�ڶ���������˫����
    char _szRecv[RECV_BUFF_SIZE] = {};

    //�������� ����ճ�� ��ְ�
    int RecvData(ClientSocket* pClient)
    {
        //���տͻ�������
        int nlen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0); //sizeof(DataHeader)
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            printf("�ͻ���<Socket=%d>���˳������������\n", (int)(pClient->sockfd()));
            return -1;
        }
        //����ȡ��ȡ�������ݿ�������Ϣ������
        memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nlen);
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
                OnNetMsg(pClient->sockfd(), header);
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
    void OnNetMsg(SOCKET cSock, DataHeader* header)
    {
        _recvCount++;
        auto t1 = _tTime.getElapseSecond();
        if (t1 >= 1.0)
        {
            printf("time<%lf>, socket<%d>, clients<%d>, recvCount<%d>\n", t1, (int)_sock, (int)_clients.size(), _recvCount);
            _recvCount = 0;
            _tTime.update();
        }
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                //printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN  ���ݳ��ȣ�%d  �û�����%s  �û����룺%s\n", cSock, login->dataLength, login->userName, login->PassWord);
                //�����ж��û������Ƿ���ȷ�Ĺ���
                //LoginResult ret;
                //SendData(cSock, &ret);
            }
            break;
            case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                //printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT  ���ݳ��ȣ�%d  �û�����%s\n", cSock, logout->dataLength, logout->userName);
                //�����ж��û������Ƿ���ȷ�Ĺ���
                //LogoutResult ret;
                //SendData(cSock, &ret);
            }
            break;
            default:
            {
                printf("<socket=%d>�յ�δ������Ϣ  ���ݳ��ȣ�%d\n", (int)_sock, header->dataLength);
                //DataHeader ret;
                //SendData(cSock, &ret);
            }
        }
    }

    //����ָ��Socket����
    int SendData(SOCKET cSock, DataHeader* header)
    {
        if (isRun() && header)
        {
            return (int)send(cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

    void SendDataToAll(DataHeader* header)
    {
        for (int n = (int)_clients.size() - 1; n >= 0; n--)
        {
            SendData(_clients[n]->sockfd(), header);
        }
    }
};

#endif //_EasyTcpServer_hpp_
