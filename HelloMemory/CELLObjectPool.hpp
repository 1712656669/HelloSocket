#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_

#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__) //������������ɱ��
	#endif
#else
	#ifndef xPrintf
		#define xPrintf(...)  //���滻Ϊ��
	#endif
#endif

template<class Type, size_t nPools>
class CELLObjectPool
{
public:
	CELLObjectPool()
	{
		InitPool();
	}

	~CELLObjectPool()
	{
		if (_pBuf)
		{
			delete[] _pBuf;
		}
	}

private:
	class NodeHeader
	{
	public:
		//��һ��λ��
		NodeHeader* pNext;
		//�ڴ����
		int nID;
		//���ô���
		char nRef;
		//�Ƿ����ڴ����
		bool bPool;
	private:
		//Ԥ�� �ڴ����
		char c1;
		char c2;
	};
	
private:
	//��ʼ�������
	void InitPool()
	{
		//����
		assert(nullptr == _pBuf);
		//�����ڴ�صĴ�С
		size_t realSize = sizeof(Type) + sizeof(NodeHeader);
		size_t bufSize = nPools * realSize;
		//��ϵͳ����ص��ڴ�
		_pBuf = new char[bufSize];
		//��ʼ���ڴ��
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->pNext = nullptr;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->bPool = true;

		//�����ڴ���ʼ��
		NodeHeader* pTemp1 = _pHeader;

		for (int n = 1; n < (int)nPools; n++)
		{
			NodeHeader* pTemp2 = (NodeHeader*)(_pBuf + n * realSize);
			pTemp2->pNext = nullptr;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->bPool = true;

			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
		
	}

public:
	//��������ڴ�
	void* allocObjMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader) //�ڴ治��
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
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
		xPrintf("allocObjMemory: %p, id=%d, size=%zd\n", pReturn, pReturn->nID, nSize);
		return (char*)pReturn + sizeof(NodeHeader);
	}

	//�ͷŶ���
	void freeObjMemory(void* pMem)
	{
		NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		xPrintf("freeObjMemory: %p, id=%d\n", pBlock, pBlock->nID);
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
				return;
			}
			delete[] pBlock;
		}
	}

private:
	//ͷ���ڴ浥Ԫ
	NodeHeader* _pHeader;
	//������ڴ滺������ַ
	char* _pBuf;
	//
	std::mutex _mutex;
};

template<class Type, size_t nPools>
class ObjectPoolBase
{
public:
	void* operator new(size_t nSize)
	{
		return ObjectPool().allocObjMemory(nSize);
	}

	void operator delete(void* p)
	{
		ObjectPool().freeObjMemory(p);
	}

	template<typename ... Args>//�������� �ɱ����
	static Type* creatObject(Args ... args)
	{
		Type* obj = new Type(args ...);
		//���������������
		return obj;
	}

	static void destroyObject(Type* obj)
	{
		delete obj;
	}

private:
	typedef CELLObjectPool<Type, nPools> classTPool;

	static classTPool& ObjectPool()
	{
		//��̬CELLObjectPool����
		static classTPool sPool;
		return sPool;
	}

};

#endif // !_CELLObjectPool_hpp_
