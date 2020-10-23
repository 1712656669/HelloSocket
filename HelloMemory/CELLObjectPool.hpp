#ifndef _CELLObjectPool_hpp_
#define _CELLObjectPool_hpp_

#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__) //宏替代函数，可变参
	#endif
#else
	#ifndef xPrintf
		#define xPrintf(...)  //被替换为空
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
		//下一块位置
		NodeHeader* pNext;
		//内存块编号
		int nID;
		//引用次数
		char nRef;
		//是否在内存池中
		bool bPool;
	private:
		//预留 内存对齐
		char c1;
		char c2;
	};
	
private:
	//初始化对象池
	void InitPool()
	{
		//断言
		assert(nullptr == _pBuf);
		//计算内存池的大小
		size_t realSize = sizeof(Type) + sizeof(NodeHeader);
		size_t bufSize = nPools * realSize;
		//向系统申请池的内存
		_pBuf = new char[bufSize];
		//初始化内存池
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->pNext = nullptr;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->bPool = true;

		//遍历内存块初始化
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
	//申请对象内存
	void* allocObjMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		NodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader) //内存不足
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

	//释放对象
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
	//头部内存单元
	NodeHeader* _pHeader;
	//对象池内存缓存区地址
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

	template<typename ... Args>//不定参数 可变参数
	static Type* creatObject(Args ... args)
	{
		Type* obj = new Type(args ...);
		//可以添加其他代码
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
		//静态CELLObjectPool对象
		static classTPool sPool;
		return sPool;
	}

};

#endif // !_CELLObjectPool_hpp_
