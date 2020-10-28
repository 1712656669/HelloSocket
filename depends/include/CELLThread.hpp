#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include "CELLSemaphore.hpp"
#include <functional>

class CELLThread
{
public:
	static void Sleep(time_t dt)
	{
		std::chrono::milliseconds t(dt);
		std::this_thread::sleep_for(t);
	}

private:
	typedef std::function<void(CELLThread*)> EventCall;
public:
	void Start(EventCall onCreate = nullptr,
		EventCall onRun = nullptr,
		EventCall onDestroy = nullptr)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_isRun)
		{
			_isRun = true;
			
			if (onCreate)
			{
				_onCreate = onCreate;
			}
			if (onRun)
			{
				_onRun = onRun;
			}
			if (onDestroy)
			{
				_onDestroy = onDestroy;
			}

			//线程
			//std::mem_fn(&CELLThread::OnWork)
			std::thread t(&CELLThread::OnWork, this);
			t.detach();
		}
	}

	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
	}

	//在工作函数中退出
	//不需要使用信号量来阻塞等待
	//如果使用会阻塞
	void Exit()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
		}
	}

	bool isRun()
	{
		return _isRun;
	}

private:
	void OnWork()
	{
		if (_onCreate)
		{
			_onCreate(this);
		}
		if (_onRun)
		{
			_onRun(this);
		}
		if (_onDestroy)
		{
			_onDestroy(this);
		}
		_sem.wakeup();
	}

private:
	EventCall _onCreate = nullptr;
	EventCall _onRun = nullptr;
	EventCall _onDestroy = nullptr;
	//不同线程中改变数据时需要加锁
	std::mutex _mutex;
	//控制线程的终止、退出
	CELLSemaphore _sem;
	//线程是否启动运行
	bool _isRun = false;
};

#endif // !_CELL_THREAD_HPP_
