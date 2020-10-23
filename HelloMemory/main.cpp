#include "Alloctor.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include "CELLTimestamp.hpp"
#include "CELLObjectPool.hpp"

using namespace std;

mutex m;
const int tCount = 4;
const int mCount = 500;
const int nCount = mCount / tCount;

class ClassA :public ObjectPoolBase<ClassA, 5>
{
public:
	ClassA(int n)
	{
		num = n;
		printf("ClassA\n");
	}

	~ClassA()
	{
		printf("~ClassA\n");
	}

	int num;
};

class ClassB :public ObjectPoolBase<ClassB, 3>
{
public:
	ClassB(int m, int n)
	{
		num = m * n;
		printf("ClassB\n");
	}

	~ClassB()
	{
		printf("~ClassB\n");
	}

private:
	int num;
};

void workFun(int index)
{
	ClassA* data[nCount];
	for (size_t i = 0; i < nCount; i++)
	{
		data[i] = ClassA::creatObject(6);
	}

	for (size_t i = 0; i < nCount; i++)
	{
		ClassA::destroyObject(data[i]);
	}
}

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
	cout << tTime.getElapseTimeInMilliSec() << endl;
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

	/*
	shared_ptr<ClassA> b = make_shared<ClassA>();
	printf("use_count=%d\n", b.use_count());
	b->num = 100;
	fun(b);
	printf("use_count=%d\n", b.use_count());
	printf("num=%d\n", b->num);
	*/

	/*
	ClassA* a1 = new ClassA(2);
	delete a1;

	ClassA* a2 = ClassA::creatObject(6);
	ClassA::destroyObject(a2);

	ClassB* b1 = new ClassB(5, 6);
	delete b1;

	ClassB* b2 = ClassB::creatObject(5, 6);
	ClassB::destroyObject(b2);
	*/

	{
		//不使用对象池，直接使用内存池
		shared_ptr<ClassA> s1 = make_shared<ClassA>(5);
	}
	printf("------------\n");
	{
		//使用对象池
		shared_ptr<ClassA> s1(new ClassA(5));
		//string str("hello");
	}
	printf("------------\n");
	ClassA* a1 = new ClassA(2);
	delete a1;
	printf("------------\n");
	return 0;
}