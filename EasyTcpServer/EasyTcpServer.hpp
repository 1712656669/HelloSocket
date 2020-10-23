#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN //��ֹwindows.h��WinSock2.h���ض���
    #define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa��������ʱ������������
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

class EasyTcpServer
{
private:
    SOCKET _sock;
    std::vector<SOCKET> g_clients;

public:
    EasyTcpServer()
    {
        _sock = INVALID_SOCKET;
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
        /*if (INVALID_SOCKET == _sock)
         {
         InitSocket();
         }*/
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
        SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
        _cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
        if (INVALID_SOCKET == _cSock)
        {
            printf("<socket=%d>���󣬽��ܵ���Ч�ͻ���SOCKET...\n", (int)_sock);
        }
        else
        {
            NewUserJoin userJoin;
            SendDataToAll(&userJoin);
            g_clients.push_back(_cSock);
            printf("<socket=%d>�¿ͻ����룺socket = %d��IP = %s\n", (int)_sock, (int)_cSock, inet_ntoa(clientAddr.sin_addr));
        }
        return _cSock;
    }

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            for (int n = (int)g_clients.size() - 1; n >= 0; n--)
            {
                closesocket(g_clients[n]);
            }
            //�ر��׽���closesocket
            closesocket(_sock);
            //���Windows socket����
            WSACleanup();
#else
            for (int n = (int)g_clients.size() - 1; n >= 0; n--)
            {
                close(g_clients[n]);
            }
            close(_sock);
#endif
            _sock = INVALID_SOCKET;
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
            for (int n = (int)g_clients.size() - 1; n >= 0; n--)
            {
                FD_SET(g_clients[n], &fdRead);
                if (maxSock < g_clients[n])
                {
                    maxSock = g_clients[n];
                }
            }
            //int nfds �����������ļ��������ķ�Χ������������
            //�������ļ������������ֵ��1����windows�������������ν������д0
            //timeout ����select()�ĳ�ʱ����ʱ��
            timeval t = { 1,0 }; //s,ms
            int ret = select((int)maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
            printf("select ret=%d count=%d\n", ret, _nCount++);
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
            }
            for (int n = (int)g_clients.size() - 1; n >= 0; n--)
            {
                if (FD_ISSET(g_clients[n], &fdRead))
                {
                    if (-1 == RecvData(g_clients[n]))
                    {
                        //std::vector<SOCKET>::iterator
                        auto iter = g_clients.begin() + n;
                        if (iter != g_clients.end())
                        {
                            g_clients.erase(iter);
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
    char szRecv[4096] = {};

    //�������� ����ճ�� ��ְ�
    int RecvData(SOCKET _cSock)
    {
        //���շ��������
        int nlen = (int)recv(_cSock, szRecv, 4096, 0); //sizeof(DataHeader)
        printf("nlen=%d\n", nlen);
        LoginResult ret;
        SendData(_cSock, &ret);
        /*
        DataHeader* header = (DataHeader*)szRecv;
        if (nlen <= 0)
        {
            printf("�ͻ���<Socket=%d>���˳������������\n", _cSock);
            return -1;
        }
        //��ַƫ��sizeof(DataHeader)
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        OnNetMsg(_cSock, header);
        */
        return 0;
    }

    //��Ӧ������Ϣ
    void OnNetMsg(SOCKET _cSock, DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN:
            {
                Login* login = (Login*)header;
                printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN  ���ݳ��ȣ�%d  �û�����%s  �û����룺%s\n", (int)_cSock, login->dataLength, login->userName, login->PassWord);
                //�����ж��û������Ƿ���ȷ�Ĺ���
                LoginResult ret;
                SendData(_cSock, &ret);
            }
            break;
            case CMD_LOGOUT:
            {
                Logout* logout = (Logout*)header;
                printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT  ���ݳ��ȣ�%d  �û�����%s\n", (int)_cSock, logout->dataLength, logout->userName);
                //�����ж��û������Ƿ���ȷ�Ĺ���
                LogoutResult ret;
                SendData(_cSock, &ret);
            }
            break;
            default:
            {
                DataHeader header = { 0,CMD_ERROR };
                SendData(_cSock, &header);
            }
        }
    }

    //����ָ��Socket����
    int SendData(SOCKET _cSock, DataHeader* header)
    {
        if (isRun() && header)
        {
            return (int)send(_cSock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

    void SendDataToAll(DataHeader* header)
    {
        for (int n = (int)g_clients.size() - 1; n >= 0; n--)
        {
            SendData(g_clients[n], header);
        }
    }
};

#endif //_EasyTcpServer_hpp_
