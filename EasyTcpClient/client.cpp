﻿#define WIN32_LEAN_AND_MEAN //防止windows.h和WinSock2.h宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS //inet_ntoa函数，过时函数重新启用
#define _CRT_SECURE_NO_WARNINGS //scanf函数和strcpy函数

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib,"ws2_32.lib")

int main()
{
    //启动Windows socket 2.x环境
    WORD ver = MAKEWORD(2, 2);
    WSADATA dat;
    WSAStartup(ver, &dat);
    //------------
    //-- 用Socket API建立简易TCP客户端
    // 1 建立一个socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == _sock)
    {
        printf("错误，建立Socket失败...\n");
    }
    else
    {
        printf("建立Socket成功...\n");
    }
    // 2 连接服务器 connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567); //host to net unsigned short
    _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
    if (SOCKET_ERROR == ret)
    {
        printf("错误，连接服务器失败...\n");
    }
    else
    {
        printf("连接服务器成功...\n");
    }

    while (true)
    {
        // 3 输入请求命令
        char cmdBuf[128] = {};
        scanf("%s", cmdBuf);
        // 4 处理请求命令
        if (0 == strcmp(cmdBuf, "exit"))
        {
            printf("收到exit命令，任务结束。\n");
            break;
        }
        else
        {
            // 5 向服务器发送请求命令
            send((int)_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
        }
        // 6 接受服务器信息 recv
        char recvBuf[128] = {};
        int nlen = recv(_sock, recvBuf, 128, 0);
        if (nlen > 0)
        {
            printf("接收到数据：%s\n", recvBuf);
        }
    }
    // 7 关闭套接字closesocket
    closesocket(_sock);
    //------------
    //清除Windows socket环境

    WSACleanup();
    printf("客户端已退出。\n");
    getchar();
    getchar();
    return 0;
}