#ifndef _CELL_HPP_
#define _CELL_HPP_

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

//自定义
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "CELLObjectPool.hpp"

#include <stdio.h>

//缓冲区最小单元大小
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
