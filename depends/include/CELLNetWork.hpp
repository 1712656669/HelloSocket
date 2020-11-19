#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_

#include "CELL.hpp"

class CELLNetWork
{
private:
	CELLNetWork()
	{
#ifdef _WIN32
        //创建WORD的版本号 和 WSADATA变量
        //高位字节指出副版本(修正)号，低位字节指明主版本号
        WORD version = MAKEWORD(2, 2);
        //操作系统用来返回请求的Socket的版本信息
        WSADATA data;
        //启动Windows socket 2.x环境
        WSAStartup(version, &data);
        //WSA异步套接字
        //同步：调用发送/接收函数，函数调用结束时，相应的发送/接收处理已经完成，数据已经被发送/接收到了
        //异步：调用发送/接收函数，函数调用结束时，表示数据已经送达底层去处理了，但是是否处理结束并不知道
        //      代码可以继续做其他操作，稍候相应的操作结束时，处理结果会通过事件（或者回调）来告诉调用者
#endif
        //
#ifndef _WIN32
        //if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        //{
        //    return (1);
        //}
        //忽略异常信号，默认情况会导致进程终止
        signal(SIGPIPE, SIG_IGN);
#endif
	}

	~CELLNetWork()
	{
#ifdef _WIN32
        //清除Windows socket环境
        WSACleanup();
#endif
	}

public:
    static void Init()
    {
        static CELLNetWork obj;
    }

};

#endif // !_CELL_NET_WORK_HPP_
