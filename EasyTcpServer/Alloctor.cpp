#include "Alloctor.hpp"
#include "MemoryMgr.hpp"

void* operator new(size_t nSize)
{
	return MemoryMgr::Instance().allocMem(nSize);
}

void operator delete(void* p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* operator new[](size_t nSize)
{
	return MemoryMgr::Instance().allocMem(nSize);
}

void operator delete[](void* p)
{
	MemoryMgr::Instance().freeMem(p);
}