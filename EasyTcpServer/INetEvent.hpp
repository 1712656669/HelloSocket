#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include "CELL.hpp"

//网络事件接口
class INetEvent
{
public:
    //客户端加入事件
    virtual void OnNetJoin(CELLClientPtr& pClient) = 0;
    //客户端离开事件
    virtual void OnNetLeave(CELLClientPtr& pClient) = 0;
    //客户端消息事件
    virtual void OnNetMsg(CELLServer* pCELLServer, CELLClientPtr& pClient, DataHeaderPtr& header) = 0;
    //recv事件
    virtual void OnNetRecv(CELLClientPtr& pClient) = 0;
};

#endif // !_I_NET_EVENT_HPP_
