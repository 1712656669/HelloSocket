#ifndef _MemoryMgr_hpp_
#define _MemoryMgr_hpp_

#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
	#include <stdio.h>
	#define xPrintf(...) printf(__VA_ARGS__) //������������ɱ��
#else
	#define xPrintf(...)  //���滻Ϊ��
#endif

#define MAX_MEMORY_SIZE 1024

class MemoryAlloc;

//�ڴ�� ��С��Ԫ
class MemoryBlock
{
public:
	//�������ڴ�飨�أ�
	MemoryAlloc* pAlloc;
	//��һ��λ��
	MemoryBlock* pNext;
	//�ڴ����
	int nID;
	//���ô���
	int nRef;
	//�Ƿ����ڴ����
	bool bPool;

private:
	//Ԥ�� �ڴ����
	char c1;
	char c2;
	char c3;

};

//�ڴ��
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		xPrintf("MemoryAlloc\n");
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlocks = 0;
	}

	~MemoryAlloc()
	{
		if (_pBuf)
		{
			free(_pBuf);
		}
	}

	//�����ڴ�
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			init_szAllocMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader) //�ڴ治��
		{
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->bPool = false;
		}
		else
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocMem: %p, id=%d, size=%zd\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	//�ͷ��ڴ�
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else
		{
			if (--pBlock->nRef != 0)
			{
				free(pBlock);
			}
		}
	}

	//��ʼ��
	void init_szAllocMemory()
	{
		xPrintf("initMemory: nSize=%zd, nBlocks=%zd\n", _nSize, _nBlocks);
		//����
		assert(nullptr == _pBuf);
		if (_pBuf)
		{
			return;
		}
		//�����ڴ�صĴ�С
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t bufSize = realSize * _nBlocks;
		//��ϵͳ����ص��ڴ�
		_pBuf = (char*)malloc(bufSize);

		//��ʼ���ڴ��
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->bPool = true;

		//�����ڴ���ʼ��
		MemoryBlock* pTemp1 = _pHeader;
		
		for (int n = 1; n < (int)_nBlocks; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + n * realSize);
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->bPool = true;

			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}

protected:
	//�ڴ�ص�ַ
	char* _pBuf;
	//ͷ���ڴ浥Ԫ
	MemoryBlock* _pHeader;
	//�ڴ浥Ԫ�Ĵ�С
	size_t _nSize;
	//�ڴ浥Ԫ������
	size_t _nBlocks;
	std::mutex _mutex;

};

//������������Ա����ʱ��ʼ��MemoryAlloc�ĳ�Ա����
template<size_t nSize, size_t nBlocks>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		//64λ n = 8  32λ n = 4
		const size_t n = sizeof(void*);
		//�����nSize������ nSize = 61   61/8=7  61%8=5
		//_nSize = 7*8 + 8 = 64
		_nSize = (nSize / n) * n + (nSize % n ? n : 0); //�ڴ����
		_nBlocks = nBlocks;
	}

};

//�ڴ������
class MemoryMgr
{
private:
	MemoryMgr()
	{
		xPrintf("MemoryMgr\n");
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
	}

	~MemoryMgr()
	{
	
	}

public:
	static MemoryMgr& Instance()
	{
		//����ģʽ
		static MemoryMgr mgr;
		return mgr;
	}

	//�����ڴ�
	void* allocMem(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->bPool = false;
			xPrintf("allocMem: %p, id=%d, size=%zd\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
	}

	//�ͷ��ڴ�
	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem:  %p, id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else
		{
			if (--pBlock->nRef == 0)
			{
				free(pBlock);
			}
		}
	}

	//�����ڴ������ü���
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}

private:
	//��ʼ���ڴ��ӳ������
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA)
	{
		for (int n = nBegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMemA;
		}
	}

private:
	MemoryAlloctor<64, 10000> _mem64;
	MemoryAlloctor<128, 10000> _mem128;
	MemoryAlloctor<256, 10000> _mem256;
	MemoryAlloctor<512, 10000> _mem512;
	MemoryAlloctor<1024, 10000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
};

#endif // _MemoryMgr_hpp_
