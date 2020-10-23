#ifndef _CELL_HPP_
#define _CELL_HPP_

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

//�Զ���
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "CELLObjectPool.hpp"

#include <stdio.h>

//��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif

class CellS2CTask;
class CellClient;
class CellServer;
typedef std::shared_ptr<CellS2CTask> CellS2CTaskPtr;
typedef std::shared_ptr<CellClient> CellClientPtr;
typedef std::shared_ptr<CellServer> CellServerPtr;
typedef std::shared_ptr<LoginResult> LoginResultPtr;
typedef std::shared_ptr<DataHeader> DataHeaderPtr;

#endif // !_CELL_HPP_
