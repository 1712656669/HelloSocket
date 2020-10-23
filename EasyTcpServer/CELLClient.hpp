#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "CELL.hpp"

//�ͻ�����������
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

    //��������
    int SendData(DataHeaderPtr header)
    {
        int ret = SOCKET_ERROR;
        //Ҫ���͵����ݳ���
        int nSendLen = header->dataLength;
        //Ҫ���͵�����
        const char* pSendData = (const char*)header.get();
        while (true)
        {
            if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
            {
                //����ɿ��������ݳ���
                int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
                //��������
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //����ʣ������λ��
                pSendData += nCopyLen;
                //����ʣ�����ݳ���
                nSendLen -= nCopyLen;
                //��������
                ret = (int)send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
                //����β��λ������
                _lastSendPos = 0;
                //���ʹ���
                if (SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else
            {
                //��Ҫ���͵����ݿ��������ͻ�����β��
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                //��������β��λ��
                _lastSendPos += nSendLen;
                break;
            }
        }
        return ret;
    }

private:
    //fd_set file desc set
    SOCKET _sockfd;
    //�ڶ������� ��Ϣ������
    char _szMsgBuf[RECV_BUFF_SIZE];
    //��Ϣ������������β��λ��
    int _lastPos;
    //�ڶ������� ���ͻ�����
    char _szSendBuf[SEND_BUFF_SIZE];
    //���ͻ�����������β��λ��
    int _lastSendPos;
};

#endif // !_CELL_CLIENT_HPP_