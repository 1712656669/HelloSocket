﻿#ifndef _CELL_TASK_H_

#include <thread>
#include <mutex>
#include <list>

//任务基类型
class CELLTask
{
public:
	CELLTask()
	{

	}

	virtual ~CELLTask()
	{
	
	}

	//执行任务
	virtual void doTask()
	{

	}

private:

};

typedef std::shared_ptr<CELLTask> CELLTaskPtr;
//执行任务的服务类型
class CELLTaskServer
{
private:
	//任务数据
	std::list<CELLTaskPtr> _tasks;
	//任务数据缓冲区
	std::list<CELLTaskPtr> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;

public:

	//添加任务
	void addTask(CELLTaskPtr task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_tasksBuf.push_back(task);
		//_mutex.unlock();
	}

	//启动工作线程
	void Start()
	{
		//线程
		//std::mem_fn(&CELLTaskServer::OnRun)
		std::thread t(&CELLTaskServer::OnRun, this);
		t.detach();
	}

protected:
	//工作函数
	void OnRun()
	{
		while (true)
		{
			//从缓冲区取出数据
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//如果没有任务
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks)
			{
				pTask->doTask();
			}
			//清空任务
			_tasks.clear();
		}
	}

};

#endif // !_CELL_TASK_H_
