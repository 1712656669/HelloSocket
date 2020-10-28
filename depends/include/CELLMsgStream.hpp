#ifndef _CELL_MSG_STREAM_HPP_
#define _CELL_MSG_STREAM_HPP_

#include "CELLStream.hpp"
#include "MessageHeader.hpp"

#include <string>

class CELLRecvStream :public CELLStream
{
public:
	CELLRecvStream(DataHeader* header)
		:CELLStream((char*)header, header->dataLength)
	{
		push(header->dataLength);
		//Ԥ�ȶ�ȡ��������
		ReadInt16();
		//Ԥ�ȶ�ȡ��Ϣ����
		getNetCmd();
	}

	uint16_t getNetCmd()
	{
		uint16_t cmd = CMD_ERROR;
		Read<uint16_t>(cmd);
		return cmd;
	}
};

class CELLSendStream :public CELLStream
{
public:
	CELLSendStream(char* pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete)
	{
		//Ԥ��ռ����Ϣ��������ռ�
		Write<uint16_t>(0);
	}

	CELLSendStream(int nSize = 1024)
		:CELLStream(nSize)
	{
		//Ԥ��ռ����Ϣ��������ռ�
		Write<uint16_t>(0);
	}

	void setNetCmd(uint16_t cmd)
	{
		Write<uint16_t>(cmd);
	}

	bool WriteString(const char* str, int len)
	{
		return WriteArray(str, len);
	}

	bool WriteString(const char* str)
	{
		return WriteArray(str, (uint32_t)strlen(str));
	}

	bool WriteString(std::string& str)
	{
		return WriteArray(str.c_str(), (uint32_t)str.length());
	}

	void finish()
	{
		int pos = length();
		setWritePos(0);
		Write<uint16_t>(pos);
		setWritePos(pos);
	}
};

#endif // !_CELL_STREAM_HPP_
