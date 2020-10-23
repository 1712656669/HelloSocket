#ifndef _CELL_TASK_H_

#include <thread>
#include <mutex>
#include <list>

//任务基类型
class CellTask
{
public:
	CellTask()
	{

	}

	virtual ~CellTask()
	{
	
	}

	//执行任务
	virtual void doTask()
	{

	}

private:

};
//执行任务的服务类型
class CellTaskServer
{
private:
	//任务数据
	std::list<CellTask*> _tasks;
	//任务数据缓冲区
	std::list<CellTask*> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;

public:
	CellTaskServer()
	{
	
	}

	~CellTaskServer()
	{
	
	}

	//添加任务
	void addTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_tasks.push_back(task);
		//_mutex.unlock();
	}

	//启动服务
	void Start()
	{

	}

	//工作函数
	void OnRun()
	{
	
	}

};

#endif // !_CELL_TASK_H_
