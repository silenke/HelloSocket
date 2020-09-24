#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_


#include "Cell.hpp"
#include "CellTask.hpp"
#include <ctime>


#pragma warning(disable : 4996)


class CELLlog
{
public:
	CELLlog()
	{
		_taskServer.Start();
	}

	~CELLlog()
	{
		_taskServer.Close();
		if (_logFile)
		{
			Info("CELLlog::setLogPath fclose(_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}

	void setLogPath(const char* logPath, const char* mode)
	{
		if (_logFile)
		{
			Info("CELLlog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
 
		if (!fopen_s(&_logFile, logPath, mode))
		{
			Info("CELLlog::setLogPath successed£¬<%s, %s>\n", logPath, mode);
		}
		else
		{
			Info("CELLlog::setLogPath failed£¬<%s, %s>\n", logPath, mode);
		}
	}

	static CELLlog& Instance()
	{
		static CELLlog sLog;
		return sLog;
	}

	static void Info(const char* pStr)
	{
		CELLlog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "Info");
				fprintf(pLog->_logFile, "¡¾%d-%d-%d %d:%d:%d¡¿",
					now->tm_year + 1900, now->tm_mon, now->tm_mday,
					now->tm_hour + 8, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, pStr);
				fflush(pLog->_logFile);
			}
			printf(pStr);
		});
	}

	template<typename ...Args>
	static void Info(const char* pFormat, Args ... args)
	{
		CELLlog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "Info");
				fprintf(pLog->_logFile, "¡¾%d-%d-%d %d:%d:%d¡¿",
					now->tm_year + 1900, now->tm_mon, now->tm_mday,
					now->tm_hour + 8, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, pFormat, args...);
				fflush(pLog->_logFile);
			}
			printf(pFormat, args...);
		});
	}

private:
	FILE* _logFile = nullptr;
	CellTaskServer _taskServer;
};


#endif // !_CELL_LOG_HPP_
