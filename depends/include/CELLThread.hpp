#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_


#include "CellSemaphore.hpp"

#include <mutex>
#include <functional>


class CellThread
{
private:
	typedef std::function<void(CellThread*)> EventCall;

public:
	// �����߳�
	void Start(EventCall onCreate = nullptr,
		EventCall onRun = nullptr, EventCall onDestroy = nullptr)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_isRun)
		{
			_isRun = true;

			if (onCreate) _onCreate = onCreate;
			if (onRun) _onRun = onRun;
			if (onDestroy) _onDestroy = onDestroy;

			std::thread t(std::mem_fun(&CellThread::OnWork), this);
			t.detach();
		}
	}

	// �ر��߳�
	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
	}

	// �ڹ����߳����˳�
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

protected:
	void OnWork()
	{
		if (_onCreate) _onCreate(this);
		if (_onRun) _onRun(this);
		if (_onDestroy) _onDestroy(this);

		_sem.wakeup();
	}

private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestroy;
	std::mutex _mutex;
	CellSemaphore _sem;
	bool _isRun = false;
};


#endif // !_CELL_THREAD_HPP_
