#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_


#include "Cell.hpp"


class CELLNetWork
{
private:
	CELLNetWork()
	{
#ifdef _WIN32
		// ��ʼ���׽��ֿ�
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(version, &data);
#endif // _WIN32

#ifndef _WIN32
		signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32

	}

	~CELLNetWork()
	{
#ifdef _WIN32
		// �����׽��ֿ�
		WSACleanup();
#endif
	}

public:
	static void Init()
	{
		static CELLNetWork obj;
	}
};


#endif // !_CELL_NET_WORK_HPP_

