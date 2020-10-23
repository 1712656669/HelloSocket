// linux环境编译命令
// g++ server.cpp -std=c++11 -pthread -o server
// ./server

#include "EasyTcpServer.hpp"

int main()
{
    EasyTcpServer server;
    server.InitSocket();
    server.Bind(nullptr, 4567);
    server.Listen(5);

    while (server.isRun())
    {
        server.OnRun();
        //printf("空闲时间处理其他业务...\n");
    }
    server.Close();
    printf("服务器已退出\n");
    getchar();
    return 0;
}
