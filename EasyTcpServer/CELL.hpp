#ifndef _CELL_HPP_
#define _CELL_HPP_

#ifdef _WIN32
	#define FD_SETSIZE    10000
	#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
	#define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数

	#include <windows.h>
	#include <WinSock2.h>
	//静态链接库，解析WSAStartup和WSACleanup
	#pragma comment(lib,"ws2_32.lib")
#else
	#include <unistd.h> //unix std，类似windows.h
	#include <arpa/inet.h> //类似WinSock2.h
	#include <string.h>
	#include <signal.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif //!_WIN32

//自定义
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "CELLObjectPool.hpp"

#include <stdio.h>

//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
	#define RECV_BUFF_SIZE 8192
	#define SEND_BUFF_SIZE 10240
#endif

class CELLS2CTask;
class CELLClient;
class CELLServer;
typedef std::shared_ptr<CELLS2CTask> CELLS2CTaskPtr;
typedef std::shared_ptr<CELLClient> CELLClientPtr;
typedef std::shared_ptr<CELLServer> CELLServerPtr;

#endif // !_CELL_HPP_
