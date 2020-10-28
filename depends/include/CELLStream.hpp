#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_

#include <cstdint>

class CELLStream
{
public:
	CELLStream(char* pData, int nSize, bool bDelete = false)
	{
		_nSize = nSize;
		_pBuff = pData;
		_nWritePos = 0;
		_nReadPos = 0;
		_bDelete = bDelete;
	}

	CELLStream(int nSize = 1024)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
		_nWritePos = 0;
		_nReadPos = 0;
		_bDelete = true;
	}

	virtual ~CELLStream()
	{
		if (_bDelete && _pBuff)
		{
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}
public:
	char* data()
	{
		return _pBuff;
	}

	int length()
	{
		return _nWritePos;
	}

	//�ܷ����n�ֽ�����
	inline bool canRead(int n)
	{
		return _nSize - _nReadPos >= n;
	}

	//�ܷ�д��n�ֽ�����
	inline bool canWrite(int n)
	{
		return _nSize - _nWritePos >= n;
	}

	//��д��λ�����n�ֽڳ���
	inline void push(int n)
	{
		_nWritePos += n;
	}

	//�Ѷ�ȡλ�����n�ֽڳ���
	inline void pop(int n)
	{
		_nReadPos += n;
	}

	inline void setWritePos(int n)
	{
		_nWritePos = n;
	}

	inline int getWritePos(int n)
	{
		return _nWritePos;
	}

	template<typename T>
	bool Read(T& n, bool bOffset = true)
	{
		//����Ҫ��ȡ���ݵĴ�С
		int nLen = sizeof(T);
		//�ж��ܲ��ܶ�ȡ
		if (canRead(nLen))
		{
			//��Ҫ��ȡ������ ��������
			memcpy(&n, _pBuff + _nReadPos, nLen);
			//�����Ѷ�ȡ����β��λ��
			if (bOffset)
			{
				pop(nLen);
			}
			return true;
		}
		return false;
	}

	template<typename T>
	bool onlyRead(T& n)
	{
		return Read(n, false);
	}

	template<typename T>
	uint32_t ReadArray(T* pArr, uint32_t len)
	{
		//��ȡ����Ԫ�ظ���������ƫ�ƶ�ȡλ��
		uint32_t len1 = 0;
		Read(len1, false);
		//�жϻ��������ܷ�ŵ���
		if (len1 < len)
		{
			//����������ֽڳ���
			int nLen = len1 * sizeof(T);
			//�ж��ܲ��ܶ���
			if (canRead(nLen + sizeof(uint32_t)))
			{
				//�����Ѷ�λ��+���鳤����ռ�ռ�
				pop(sizeof(uint32_t));
				//��Ҫд������� ������������β��
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				//�����Ѷ�ȡ����β��λ��
				pop(nLen);
				return len1;
			}
		}
		return 0;
	}

	//char
	int8_t ReadInt8(int8_t def = 0)
	{
		Read(def);
		return def;
	}

	//short
	int16_t ReadInt16(int16_t n = 0)
	{
		Read(n);
		return n;
	}

	//int
	int32_t ReadInt32(int32_t n = 0)
	{
		Read(n);
		return n;
	}

	float ReadFloat(float n = 0.0f)
	{
		Read(n);
		return n;
	}

	double ReadDouble(double n = 0.0f)
	{
		Read(n);
		return n;
	}

	template<typename T>
	bool Write(T n)
	{
		//����Ҫд�����ݵĴ�С
		int nLen = sizeof(T);
		//�ж��ܲ���д��
		if (canWrite(nLen))
		{
			//��Ҫд������� ������������β��
			memcpy(_pBuff + _nWritePos, &n, nLen);
			//������д������β��λ��
			push(nLen);
			return true;
		}
		return false;
	}

	template<typename T>
	bool WriteArray(T* pData, uint32_t len)
	{
		//����Ҫд��������ֽڳ���
		int nLen = sizeof(T) * len;
		//�ж��ܲ���д��
		if (canWrite(nLen + sizeof(uint32_t)))
		{
			//��д������ĳ���
			WriteInt32(len);
			//��Ҫд������� ������������β��
			memcpy(_pBuff + _nWritePos, pData, nLen);
			//��������β��λ��
			push(nLen);
			return true;
		}
		return false;
	}

	bool WriteInt8(int8_t n)
	{
		return Write(n);
	}

	bool WriteInt16(int16_t n)
	{
		return Write(n);
	}

	bool WriteInt32(int32_t n)
	{
		//return Write<int32_t>(n);
		return Write(n);
	}

	bool WriteFloat(float n)
	{
		return Write(n);
	}

	bool WriteDouble(double n)
	{
		return Write(n);
	}

private:
	//���ݻ�����
	char* _pBuff = nullptr;
	//�������ܵĿռ��С���ֽڳ���
	int _nSize;
	//��д�����ݵ�β��λ�ã���д�����ݳ���
	int _nWritePos;
	//�Ѷ�ȡ���ݵ�β��λ��
	int _nReadPos;
	//pBuff���ⲿ��������ݿ�ʱ�Ƿ�Ӧ�ñ��ͷ�
	bool _bDelete;
};

#endif // !_CELL_STREAM_HPP_
