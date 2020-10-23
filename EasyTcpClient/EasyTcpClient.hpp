#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

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
#include "MessageHeader.hpp"

class EasyTcpClient
{
private:
    SOCKET _sock;
    bool _isConnect;

public:
    EasyTcpClient()
    {
        _sock = INVALID_SOCKET;
        _isConnect = false;
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
            //printf("����<socket=%d>�ɹ�...\n", (int)_sock);
        }
        return _sock;
    }

    //���ӷ�����
    int Connect(const char* ip, unsigned short port)
    {
        if (INVALID_SOCKET == _sock)
        {
            InitSocket();
        }
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
            _isConnect = true;
            //printf("<socket=%d>���ӷ�����<%s, %d>�ɹ�...\n", (int)_sock, ip, port);
        }
        return ret;
    }

    //��������
    int SendData(DataHeader* header)
    {
        int ret = SOCKET_ERROR;
        if (isRun() && header)
        {
            ret = (int)send(_sock, (const char*)header, header->dataLength, 0);
            if (SOCKET_ERROR == ret)
            {
                Close();
            }
        }
        return ret;
    }

    //����������Ϣ
    bool OnRun()
    {
        if (isRun())
        {
            fd_set fdReads;
            FD_ZERO(&fdReads);
            FD_SET(_sock, &fdReads);
            timeval t = { 0,0 }; //s,ms
            int ret = select((int)_sock + 1, &fdReads, 0, 0, &t);
            if (ret < 0) //select����
            {
                printf("<socket=%d>select�������1\n", (int)_sock);
                Close();
                return false;
            }
            if (FD_ISSET(_sock, &fdReads))
            {
                FD_CLR(_sock, &fdReads);
                if (-1 == RecvData(_sock)) //��������Ͽ�����
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

    //��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif
    //���ջ�����
    //char _szRecv[RECV_BUFF_SIZE] = {};
    //�ڶ������� ��Ϣ������
    char _szMsgBuf[RECV_BUFF_SIZE * 5] = {};
    //��Ϣ������������β��λ��
    int _lastPos = 0;

    //�������� ����ճ�� ��ְ�
    int RecvData(SOCKET cSock)
    {
        //���շ��������
        char* szRecv = _szMsgBuf + _lastPos;
        int nlen = (int)recv(cSock, szRecv, RECV_BUFF_SIZE * 5 - _lastPos, 0); //������ʵ��copy���ֽ���
        //printf("nlen=%d\n", nlen);
        if (nlen <= 0)
        {
            printf("<socket=%d>��������Ͽ����ӣ����������\n", (int)cSock);
            return -1;
        }
        //����ȡ��ȡ�������ݿ�������Ϣ������
        //memcpy(_szMsgBuf + _lastPos, _szRecv, nlen);
        //��Ϣ������������β��λ�ú���
        _lastPos += nlen;
        //�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
        while (_lastPos >= sizeof(DataHeader))
        {
            //��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
            DataHeader* header = (DataHeader*)_szMsgBuf;
            //�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
            if (_lastPos >= header->dataLength)
            {
                //ʣ��δ������Ϣ��������Ϣ�ĳ���
                int nSize = _lastPos - header->dataLength;
                //����������Ϣ
                OnNetMsg(header);
                //����Ϣ������ʣ��δ��������ǰ��
                memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
                _lastPos = nSize;
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
    void OnNetMsg(DataHeader* header)
    {
        switch (header->cmd)
        {
            case CMD_LOGIN_RESULT:
            {
                //LoginResult* login = (LoginResult*)header;
                //printf("<socket=%d>�յ���������Ϣ����CMD_LOGIN_RESULT  ���ݳ��ȣ�%d\n", (int)_sock, login->dataLength);
            }
            break;
            case CMD_LOGOUT_RESULT:
            {
                //LogoutResult* logout = (LogoutResult*)header;
                //printf("<socket=%d>�յ���������Ϣ����CMD_LOGOUT_RESULT  ���ݳ��ȣ�%d\n", (int)_sock, logout->dataLength);
            }
            break;
            case CMD_NEW_USER_JOIN:
            {
                //NewUserJoin* userJoin = (NewUserJoin*)header;
                //printf("<socket=%d>�յ���������Ϣ����CMD_NEW_USER_JOIN  ���ݳ��ȣ�%d\n", (int)_sock, userJoin->dataLength);
            }
            break;
            case CMD_ERROR:
            {
                printf("<socket=%d>�յ���������Ϣ����CMD_ERROR  ���ݳ��ȣ�%d\n", (int)_sock, header->dataLength);
            }
            break;
            default:
            {
                printf("<socket=%d>�յ�δ������Ϣ  ���ݳ��ȣ�%d\n", (int)_sock, header->dataLength);
            }
        }
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
        _isConnect = false;
    }

    //�Ƿ�����
    bool isRun()
    {
        return INVALID_SOCKET != _sock && _isConnect;
    }
};

#endif //_EasyTcpClient_hpp_
