#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_


#include "CELLThread.hpp"

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
		_thread.Start(nullptr, [this](CELLThread* pThread) {OnRun(pThread); });
	}

	void Close()
	{
		std::cout << "\t\t\tCellTaskServer<" << serverId
			<< ">.Close begin" << std::endl;
		_thread.Close();
		std::cout << "\t\t\tCellTaskServer<" << serverId
			<< ">.Close end" << std::endl;
	}

protected:
	// 工作函数
	void OnRun(CELLThread* pThread)
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
		std::cout << "\t\t\t\tCellTaskServer<" << serverId
			<< ">.OnRun exit" << std::endl;
	}

public:
	int serverId = -1;

private:
	std::list<CellTask> _tasks;
	std::list<CellTask> _tasksBuf;
	std::mutex _mutex;
	bool _isWaitExit = false;
	CELLThread _thread;
};


#endif // _CELL_TASK_H_
