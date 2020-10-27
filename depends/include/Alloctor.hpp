#ifndef _ALLOCTOR_HPP_
#define _ALLOCTOR_HPP_

#include <cstddef>

void* operator new(size_t size);
void operator delete(void* p);
void* operator new[](size_t size);
void operator delete[](void* p);

#endif // !_ALLOCTOR_HPP_