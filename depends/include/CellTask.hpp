#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_


#include "CellThread.hpp"

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
		_thread.Start(nullptr, [this](CellThread* pThread) {OnRun(pThread); });
	}

	void Close()
	{
		//CELLlog::Info("\t\t\tCellTaskServer<%d>.Close begin\n", serverId);
		_thread.Close();
		//CELLlog::Info("\t\t\tCellTaskServer<%d>.Close end\n", serverId);
	}

protected:
	// 工作函数
	void OnRun(CellThread* pThread)
	{
		while (pThread->isRun())
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
		for (auto pTask : _tasksBuf)
		{
			pTask();
		}
		_tasks.clear();

		//CELLlog::Info("\t\t\t\tCellTaskServer<%d>.OnRun exit\n", serverId);
	}

public:
	int serverId = -1;

private:
	std::list<CellTask> _tasks;
	std::list<CellTask> _tasksBuf;
	std::mutex _mutex;
	bool _isWaitExit = false;
	CellThread _thread;
};


#endif // _CELL_TASK_H_
