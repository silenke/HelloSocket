#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_


#include "CELLSemaphore.hpp"

#include <thread>
#include <mutex>
#include <list>
#include <functional>


class CellTaskServer
{
	typedef std::function<void()> CellTask;

public:
	// 添加任务
	void addTask(CellTask pTask)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(pTask);
	}

	// 启动服务
	void Start()
	{
		_isRun = true;
		std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
		t.detach();
	}

	void Close()
	{
		std::cout << "\t\t\tCellTaskServer<" << serverId
			<< ">.Close begin" << std::endl;
		if (_isRun)
		{
			_isRun = false;
			_sem.wait();
		}
		std::cout << "\t\t\tCellTaskServer<" << serverId
			<< ">.Close end" << std::endl;
	}

protected:
	// 工作函数
	void OnRun()
	{
		while (_isRun)
		{
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}

			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			for (auto pTask : _tasks)
			{
				pTask();
			}
			_tasks.clear();
		}
		std::cout << "\t\t\t\tCellTaskServer<" << serverId
			<< ">.OnRun exit" << std::endl;
		_sem.wakeup();
	}

public:
	int serverId = -1;

private:
	std::list<CellTask> _tasks;
	std::list<CellTask> _tasksBuf;
	std::mutex _mutex;
	bool _isRun = false;
	bool _isWaitExit = false;
	CELLSemaphore _sem;
};


#endif // _CELL_TASK_H_
