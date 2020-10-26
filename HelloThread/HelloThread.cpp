#include <iostream>
#include <thread>
#include <mutex>
#include <atomic> //原子
#include "CELLTimestamp.hpp"

using namespace std;

mutex m;
const int tCount = 4;
atomic_int sum;
void workFun(int index)
{
	for (int n = 0; n < 200000; n++)
	{
		//自解锁
		//lock_guard<mutex> lg(m);
		//m.lock();
		sum++;
		//m.unlock();
	}
	//cout << index << "Hello,other thread." << n << endl;
}

int main()
{
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
	cout << tTime.getElapseTimeInMicroSec() << ",sum=" << sum << endl;
	sum = 0;
	tTime.update();
	for (int n = 0; n < 800000; n++)
	{
		sum++;
	}
	cout << tTime.getElapseTimeInMicroSec() << ",sum=" << sum << endl;
	cout << "Hello,main thread." << endl;
	return 0;
}