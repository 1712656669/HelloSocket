// linux环境编译命令
// g++ server.cpp -std=c++11 -pthread -o server
// ./server

#include "EasyTcpServer.hpp"

int processor(SOCKET _cSock)
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
    
    return 0;
}

int main()
{

    //bind 绑定用于接受客户端连接的网络端口
    
    //listen 监听网络端口


    while (true)
    {
        
        //printf("空闲时间处理其他业务...\n");
    }


    printf("服务器已退出\n");
    getchar();
    return 0;
}
