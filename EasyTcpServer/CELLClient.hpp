#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "CELL.hpp"

//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME 60000 //毫秒
//在间隔指定时间后
//把发送缓冲区内缓存的数据数据发送给客户端
#define CLIENT_SEND_BUFF_TIME 200 //毫秒

//客户端数据类型
class CELLClient :public ObjectPoolBase<CELLClient, 4>
{
public:
    CELLClient(SOCKET sockfd = INVALID_SOCKET)
    {
        static int n = 1;
        id = n++;

        _sockfd = sockfd;
        memset(_szMsgBuf, 0, RECV_BUFF_SIZE);

        _lastPos = 0;
        memset(_szSendBuf, 0, SEND_BUFF_SIZE);
        _lastSendPos = 0;
        _sendBuffFullCount = 0;

        resetDTHeart();
        resetDTSend();
    }

    ~CELLClient()
    {
        printf("s=%d CELLClient%d.~CELLClient\n", serverId, id);
        if(INVALID_SOCKET != _sockfd)
        {
#ifdef _WIN32
            closesocket(_sockfd);
#else
            closes(_sockfd);
#endif // !_WIN32
            _sockfd = INVALID_SOCKET;
        }
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }

    void setLastPos(int pos)
    {
        _lastPos = pos;
    }

    //立即将发送缓冲区的数据发送给客户端
    int SendDataReal()
    {
        int ret = 0;
        //缓冲区有数据
        if (_lastSendPos > 0 && INVALID_SOCKET != _sockfd)
        {
            ret = (int)send(_sockfd, _szSendBuf, _lastSendPos, 0);
            //数据尾部位置清零
            _lastSendPos = 0;
            //
            _sendBuffFullCount = 0;
            //
            resetDTSend();
        }
        return ret;
    }

    //缓冲区的控制根据业务需求的差异而调整
    //发送数据
    int SendData(DataHeader* header)
    {
        int ret = SOCKET_ERROR;
        //要发送的数据长度
        int nSendLen = header->dataLength;
        //要发送的数据
        const char* pSendData = (const char*)header;

        if (_lastSendPos + nSendLen <= SEND_BUFF_SIZE)
        {
            //将要发送的数据拷贝到发送缓冲区尾部
            memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
            //计算数据尾部位置
            _lastSendPos += nSendLen;

            if (_sendBuffFullCount == SEND_BUFF_SIZE)
            {
                _sendBuffFullCount++;
            }

            return nSendLen;
        }
        else
        {
            _sendBuffFullCount++;
        }

        return ret;
    }

    void resetDTHeart()
    {
        _dtHeart = 0;
    }

    void resetDTSend()
    {
        _dtSend = 0;
    }

    //心跳检测
    bool checkHeart(time_t dt)
    {
        _dtHeart += dt;
        if (_dtHeart >= CLIENT_HEART_DEAD_TIME)
        {
            printf("checkHeart dead:s=%d, time=%lld\n", (int)_sockfd, _dtHeart);
            return true;
        }
        return false;
    }

    //定时发送消息检测
    bool checkSend(time_t dt)
    {
        _dtSend += dt;
        if (_dtSend >= CLIENT_SEND_BUFF_TIME)
        {
            //printf("checkSend:s=%d, time=%lld\n", _sockfd, _dtSend);
            //立即将缓冲区的数据发送出去
            SendDataReal();
            //重置发送计时
            resetDTSend();
            return true;
        }
        return false;
    }

private:
    //fd_set file desc set
    SOCKET _sockfd;
    //第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE];
    //消息缓冲区的数据尾部位置
    int _lastPos;
    //第二缓冲区 发送缓冲区
    char _szSendBuf[SEND_BUFF_SIZE];
    //发送缓冲区的数据尾部位置
    int _lastSendPos;
    //心跳死亡计时
    time_t _dtHeart;
    //上次发送消息数据时间
    time_t _dtSend;
    //发送缓冲区写满情况计数
    int _sendBuffFullCount;
public:
    int id;
    //所属server id
    int serverId = -1;
};

#endif // !_CELL_CLIENT_HPP_