#ifndef _CELL_TASK_H_

#include <thread>
#include <mutex>
#include <list>

//���������
class CellTask
{
public:
	CellTask()
	{

	}

	virtual ~CellTask()
	{
	
	}

	//ִ������
	virtual void doTask()
	{

	}

private:

};
//ִ������ķ�������
class CellTaskServer
{
private:
	//��������
	std::list<CellTask*> _tasks;
	//�������ݻ�����
	std::list<CellTask*> _tasksBuf;
	//�ı����ݻ�����ʱ��Ҫ����
	std::mutex _mutex;

public:
	CellTaskServer()
	{
	
	}

	~CellTaskServer()
	{
	
	}

	//�������
	void addTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_tasks.push_back(task);
		//_mutex.unlock();
	}

	//��������
	void Start()
	{

	}

	//��������
	void OnRun()
	{
	
	}

};

#endif // !_CELL_TASK_H_
