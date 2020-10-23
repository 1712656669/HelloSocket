#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
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

	}

	virtual ~EasyTcpServer()
	{

	}

	//初始化Socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (isRun())
		{
			printf("<socket=%d>关闭旧连接...\n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("<socket=%d>错误，建立Socket失败...\n", (int)_sock);
		}
		else
		{
			printf("建立<socket=%d>成功...\n", (int)_sock);
		}
		return _sock;
	}

	//绑定IP和端口号
	int Bind(const char* ip, unsigned short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET; //地址族(IPV4)
		_sin.sin_port = htons(4567); //端口号，host to net unsigned short
		
#ifdef _WIN32
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); //本机地址
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip)
		{
			_sin.sin_addr.s_addr = inet_addr("127.0.0.1"); //本机地址
		}
		else
		{
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("错误，绑定网络端口<%d>失败...\n", port);
		}
		else
		{
			printf("绑定网络端口<%d>成功...\n", port);
		}
		return ret;
	}

	//监听端口号
	int Listen(int n) //等待连接队列的最大长度
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("<socket=%d>错误，监听网络端口失败\n", (int)_sock);
		}
		else
		{
			printf("<socket=%d>监听网络端口成功...\n", (int)_sock);
		}
		return ret;
	}

	//接受客户端连接
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
			printf("<socket=%d>错误，接受到无效客户端SOCKET...\n", (int)_sock);
		}
		else
		{
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			g_clients.push_back(_cSock);
			printf("<socket=%d>新客户加入：socket = %d，IP = %s\n", (int)_sock, (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return _cSock;
	}

	//关闭Socket
	void Close()
	{
		if (isRun())
		{
#ifdef _WIN32
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				closesocket(g_clients[n]);
			}
			//关闭套接字closesocket
			closesocket(_sock);
			//清除Windows socket环境
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

	//处理网络消息
	bool OnRun()
	{
		if (isRun())
		{
			//伯克利套接字 BSD socket
			fd_set fdRead; //select监视的可读文件句柄集合
			fd_set fdWrite; //select监视的可写文件句柄集合
			fd_set fdExp; //select监视的异常文件句柄集合
			//将fd_set清零使集合中不含任何SOCKET
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将SOCKET加入fd_set集合
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
			//int nfds 集合中所有文件描述符的范围，而不是数量
			//即所有文件描述符的最大值加1，在windows中这个参数无所谓，可以写0
			//timeout 本次select()的超时结束时间
			timeval t = { 1,0 }; //s,ms
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0)
			{
				printf("select任务结束。\n");
				Close();
				return false;
			}
			//判断描述符(socket)是否在集合中
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				//accept 等待接受客户端连接

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

	//是否工作中
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		//缓冲区
		char szRecv[4096] = {};
		//接收服务端数据
		int nlen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nlen <= 0)
		{
			printf("客户端<Socket=%d>已退出，任务结束。\n", _cSock);
			return -1;
		}
		//地址偏移sizeof(DataHeader)
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_cSock, header);
		return 0;
	}

	//响应网络消息
	void OnNetMsg(SOCKET _cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("收到客户端<Socket=%d>请求：CMD_LOGIN  数据长度：%d  用户名：%s  用户密码：%s\n", _cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			printf("收到客户端<Socket=%d>请求：CMD_LOGOUT  数据长度：%d  用户名：%s\n", _cSock, logout->dataLength, logout->userName);
			//忽略判断用户密码是否正确的过程
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
		}
		break;
		default:
		{
			DataHeader header = { 0,CMD_ERROR };
			send(_cSock, (char*)&header, sizeof(DataHeader), 0);
		}
		}
	}

	//发送指定Socket数据
	int SendData(SOCKET _cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_cSock, (const char*)header, header->dataLength, 0);
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