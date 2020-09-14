#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_


#include "Cell.hpp"

#include <iostream>


#define CLIENT_HEART_DEAD_TIME 5000


// 客户端数据类型
class CellClient
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		resetDTHeart();
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
	int SendData(netmsg_DataHeader* header)
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

	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	bool checkHeart(time_t dt)
	{
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME)
		{
			std::cout << "checkHeart dead：socket=" << _sockfd
				<< "，time=" << _dtHeart << std::endl;
			return true;
		}
		return false;
	}

private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;
	char _szSendBuf[SEND_BUFF_SIZE * 10]{};
	int _lastSendPos = 0;
	time_t _dtHeart;
};


#endif // !_CELL_CLIENT_HPP_
