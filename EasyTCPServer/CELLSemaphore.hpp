#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_


#include <mutex>
#include <condition_variable>


class CellSemaphore
{
public:
	CellSemaphore()
	{

	}

	~CellSemaphore()
	{

	}

	void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0)
		{
			_cv.wait(lock, [this]() {return _wakeup > 0; });
			--_wakeup;
		}
	}

	void wakeup()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (++_wait <= 0)
		{
			++_wakeup;
			_cv.notify_one();
		}
	}

private:
	std::mutex _mutex;
	std::condition_variable _cv;
	int _wait = 0;		// 等待计数
	int _wakeup = 0;	// 唤醒计数
};


#endif // !_CELL_SEMAPHORE_HPP_
