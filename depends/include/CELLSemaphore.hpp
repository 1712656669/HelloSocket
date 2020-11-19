#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include <chrono>
#include <thread>
#include <condition_variable>

//信号量 互斥锁
class CELLSemaphore
{
public:
	//阻塞当前线程
	void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0)
		{
			//阻塞等待
			//在线程被阻塞时，该函数会自动调用 lck.unlock() 释放锁，使得其他被阻塞在锁竞争上的线程得以继续执行
			//一旦当前线程获得通知(notified，通常是另外某个线程调用 notify_* 唤醒了当前线程)
			//wait() 函数也是自动调用 lck.lock()，使得 lck 的状态和 wait 函数被调用时相同。
			//带条件的被阻塞:只有当条件为false时调用该wait函数才会阻塞当前线程，并且在收到其它线程的通知后只有当条件为true时才会被解除阻塞
			_cv.wait(lock, [this]()->bool {
				return _wakeup > 0;
			});
			--_wakeup;
		}
	}

	//唤醒当前线程
	void wakeup()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
		else
		{
			//CELLLog::Info("CELLSemaphore wakeup error.");
		}
	}

private:
	std::mutex _mutex;
	//阻塞等待-条件变量
	std::condition_variable _cv;
	//等待计数
	int _wait = 0;
	//唤醒计数
	int _wakeup = 0;
};

#endif // !_CELL_SEMAPHORE_HPP_
