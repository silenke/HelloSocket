#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include <chrono>

using namespace std::chrono;

class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		update();
	}

	~CELLTimestamp() {}

	void update()
	{
		_begin = high_resolution_clock::now();
	}

	long long getElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

	double getElapsedTimeInMilliSec()
	{
		return getElapsedTimeInMicroSec() * 0.001;
	}

	double getElapsedSecond()
	{
		return getElapsedTimeInMicroSec() * 0.000001;
	}

private:
	time_point<high_resolution_clock> _begin;
};

#endif // _CELLTimestamp_hpp_