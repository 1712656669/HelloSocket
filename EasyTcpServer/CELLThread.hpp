#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include "CELLSemaphore.hpp"
#include <functional>

class CELLThread
{
private:
	typedef std::function<void(CELLThread*)> EventCall;
public:
	void Start(EventCall onCreat = nullptr,
		EventCall onRun = nullptr,
		EventCall onClose = nullptr)
	{
		if (!_isRun)
		{
			_isRun = true;
			//线程
			//std::mem_fn(&CELLThread::OnWork)
			std::thread t(&CELLThread::OnWork, this);
			t.detach();
			if (onCreate)
			{
				_onCreate = onCreate;
			}
			if (onRun)
			{
				_onRun = onRun;
			}
			if (onClose)
			{
				_onClose = onClose;
			}
		}
	}

	void Close()
	{

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
		if (_onClose)
		{
			_onClose(this);
		}
	}

private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onClose;
	//控制线程的终止、退出
	CELLSemaphore _sem;
	//线程是否启动运行
	bool _isRun = false;
};

#endif // !_CELL_THREAD_HPP_
