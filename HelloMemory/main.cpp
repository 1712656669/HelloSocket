#include "Alloctor.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include "CELLTimestamp.hpp"

using namespace std;

mutex m;
const int tCount = 8;
const int mCount = 10000;
const int nCount = mCount / tCount;
void workFun(int index)
{
	char* data[nCount];
	for (size_t i = 0; i < nCount; i++)
	{
		data[i] = new char[rand() % 128 + 1];
	}

	for (size_t i = 0; i < nCount; i++)
	{
		delete[] data[i];
	}
}

class ClassA
{
public:
	ClassA()
	{
		printf("ClassA\n");
	}

	~ClassA()
	{
		printf("~ClassA\n");
	}

	int num = 0;
};

void fun(shared_ptr<ClassA> pA)
{
	printf("use_count=%d\n", pA.use_count());
	pA->num++;
}

int main()
{
	/*
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFun, n);
	}
	CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		//t[n].detach();
		t[n].join();
	}
	cout << tTime.getElapseTimeInMicroSec() << endl;
	cout << "Hello,main thread." << endl;
	*/

	/*
	int* a = new int;
	*a = 100;
	printf("a=%d\n", *a);
	delete a;

	shared_ptr<int> b = make_shared<int>();
	*b = 100;
	printf("b=%d\n", *b);
	*/

	shared_ptr<ClassA> b = make_shared<ClassA>();
	printf("use_count=%d\n", b.use_count());
	b->num = 100;
	fun(b);
	printf("use_count=%d\n", b.use_count());
	printf("num=%d\n", b->num);
	return 0;
}