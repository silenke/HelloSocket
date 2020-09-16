#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_


#include <mutex>


class CELLSemaphore
{
public:
	CELLSemaphore()
	{

	}

	~CELLSemaphore()
	{

	}

	void wait()
	{
		_isWaitExit = true;
		while (_isWaitExit)	// ×èÈûµÈ´ý
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	void wakeup()
	{
		if (_isWaitExit)
			_isWaitExit = false;
		else
			std::cout << "CELLSemaphore wakeup error." << std::endl;
	}

private:
	bool _isWaitExit = false;
};


#endif // !_CELL_SEMAPHORE_HPP_
