#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
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
#include "MessageHeader.hpp"

class EasyTcpClient
{
private:
    SOCKET _sock;

public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
    }

    virtual ~EasyTcpClient()
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

    //���ӷ�����
    int Connect(const char* ip, unsigned short port)
    {
        //if (INVALID_SOCKET == _sock)
        //{
        //    InitSocket();
        //}
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port); //host to net unsigned short
#ifdef _WIN32
        _sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
        _sin.sin_addr.s_addr = inet_addr(ip);
#endif
        int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
        if (SOCKET_ERROR == ret)
        {
            printf("<socket=%d>�������ӷ�����<%s, %d>ʧ��...\n", (int)_sock, ip, port);
        }
        else
        {
            printf("<socket=%d>���ӷ�����<%s, %d>�ɹ�...\n", (int)_sock, ip, port);
        }
        return ret;
    }

    //����������Ϣ
    int _nCount = 0;
    bool OnRun()
    {
        if (isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock, &fdReads);
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_sock + 1, &fdReads, 0, 0, &t);
            printf("select ret=%d count=%d\n", ret, _nCount++);
            if (ret < 0)
            {
                printf("<socket=%d>select�������1\n", (int)_sock);
                Close();
                return false;
            }
            if (FD_ISSET(_sock, &fdReads))
            {
                FD_CLR(_sock, &fdReads);
                if (-1 == RecvData(_sock))
                {
                    printf("<socket=%d>select�������2\n", (int)_sock);
                    Close();
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    //�ڶ���������˫����
    char szRecv[4096] = {};

    //�������� ����ճ�� ��ְ�
    int RecvData(SOCKET _cSock)
    {
        //���շ��������
        int nlen = (int)recv(_cSock, szRecv, 4096, 0); //sizeof(DataHeader)
        printf("nlen=%d\n", nlen);
        /*
        DataHeader* header = (DataHeader*)szRecv;
        if (nlen <= 0)
        {
            printf("<socket=%d>��������Ͽ����ӣ����������\n", (int)_sock);
            return -1;
        }
        //��ַƫ��sizeof(DataHeader)
        recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
        OnNetMsg(header);
        */
        return 0;
    }

    //��Ӧ������Ϣ
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN_RESULT:
            {
                LoginResult* login = (LoginResult*)header;
                printf("<socket=%d>�յ���������Ϣ����CMD_LOGIN_RESULT  ���ݳ��ȣ�%d\n", (int)_sock, login->dataLength);
            }
            break;
            case CMD_LOGOUT_RESULT:
            {
                LogoutResult* logout = (LogoutResult*)header;
                printf("<socket=%d>�յ���������Ϣ����CMD_LOGOUT_RESULT  ���ݳ��ȣ�%d\n", (int)_sock, logout->dataLength);
            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                NewUserJoin* userJoin = (NewUserJoin*)header;
                printf("<socket=%d>�յ���������Ϣ����CMD_NEW_USER_JOIN  ���ݳ��ȣ�%d\n", (int)_sock, userJoin->dataLength);
            }
            break;
        }
    }

    //��������
    int SendData(DataHeader* header)
    {
        if (isRun() && header)
        {
            return (int)send(_sock, (const char*)header, header->dataLength, 0);
        }
        return SOCKET_ERROR;
    }

    //�ر�Socket
    void Close()
    {
        if (isRun())
        {
#ifdef _WIN32
            closesocket(_sock);
            //���Windows socket����
            WSACleanup();
#else
            close(_sock);
#endif
            _sock = INVALID_SOCKET;
        }
    }

    //�Ƿ�����
    bool isRun()
    {
        return INVALID_SOCKET != _sock;
    }
};

#endif //_EasyTcpClient_hpp_
