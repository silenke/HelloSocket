#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "Cell.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>


// 客户端数据类型
class CellClient
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
	}

	SOCKET sockfd()
	{
		return _sockfd;
	}

	char* msgBuf()
	{
		return _szMsgBuf;
	}

	int getLast()
	{
		return _lastPos;
	}

	void setLast(int pos)
	{
		_lastPos = pos;
	}

	// 发送消息
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;

		int nSendLen = header->len;
		const char* pSendData = (const char*)header;

		while (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
		{
			int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
			memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
			ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
			pSendData += nCopyLen;
			nSendLen -= nCopyLen;
			_lastSendPos = 0;
			if (SOCKET_ERROR == ret)
			{
				return ret;
			}
		}
		memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
		_lastSendPos += nSendLen;

		return ret;
	}

private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;
	char _szSendBuf[SEND_BUFF_SIZE * 10]{};
	int _lastSendPos = 0;
};


#endif // !_CELL_CLIENT_HPP_
