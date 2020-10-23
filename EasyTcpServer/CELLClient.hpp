#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "CELL.hpp"

//客户端数据类型
class CellClient :public ObjectPoolBase<CellClient, 10>
{
public:
    CellClient(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
        _lastPos = 0;
        memset(_szSendBuf, 0, SEND_BUFF_SIZE);
        _lastSendPos = 0;
    }

    ~CellClient()
    {

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

    //发送数据
    int SendData(DataHeaderPtr header)
    {
        int ret = SOCKET_ERROR;
        //要发送的数据长度
        int nSendLen = header->dataLength;
        //要发送的数据
        const char* pSendData = (const char*)header.get();
        while (true)
        {
            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
            {
                //计算可拷贝的数据长度
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //拷贝数据
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //计算剩余数据位置
                pSendData += nCopyLen;
                //计算剩余数据长度
                nSendLen -= nCopyLen;
                //发送数据
                ret = (int)send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
                //数据尾部位置清零
                _lastSendPos = 0;
                //发送错误
                if (SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else
            {
                //将要发送的数据拷贝到发送缓冲区尾部
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                //计算数据尾部位置
                _lastSendPos += nSendLen;
                break;
            }
        }
        return ret;
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
};

#endif // !_CELL_CLIENT_HPP_