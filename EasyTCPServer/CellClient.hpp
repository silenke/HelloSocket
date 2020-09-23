#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_


#include "Cell.hpp"

#include <iostream>


#define CLIENT_HEART_DEAD_TIME 60000
#define CLIENT_SEND_BUFF_TIME 200


// 客户端数据类型
class CellClient
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		static int n = 0;
		id = n++;

		_sockfd = sockfd;
		resetDTHeart();
		resetDTSend();
	}

	~CellClient()
	{
		std::cout << "\t\t\t\tserver<" << serverId
			<< ">.CellClient<" << id
			<< ">.~CellClient" << std::endl;

		if (INVALID_SOCKET == _sockfd)
		{
			return;
		}

#ifdef _WIN32
		closesocket(_sockfd);
#else
		close(_sockfd);
#endif
		_sockfd = INVALID_SOCKET;
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

	// 立即发送数据
	void SendDataReal(netmsg_DataHeader* header)
	{
		SendData(header);
		SendDataReal();
	}

	// 立即发送缓冲区数据
	int SendDataReal()
	{
		int ret = 0;

		if (_lastSendPos > 0 && INVALID_SOCKET != _sockfd)
		{
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			_lastSendPos = 0;
			_sendBuffFullCount = 0;
			resetDTSend();
		}
		return ret;
	}

	// 发送消息
	int SendData(netmsg_DataHeader* header)
	{
		int ret = SOCKET_ERROR;

		int nSendLen = header->len;
		const char* pSendData = (const char*)header;

		if (_lastSendPos + nSendLen <= SEND_BUFF_SIZE)
		{
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			_lastSendPos += nSendLen;

			if (SEND_BUFF_SIZE == _lastSendPos)
			{
				_sendBuffFullCount++;
			}

			return nSendLen;
		}
		else
		{
			_sendBuffFullCount++;
		}

		return ret;
	}

	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	void resetDTSend()
	{
		_dtSend = 0;
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

	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{
			//std::cout << "checkSend：socket=" << _sockfd
			//	<< "，time=" << _dtHeart << std::endl;

			SendDataReal();
			resetDTSend();
			return true;
		}
		return false;
	}
	
public:
	int serverId = -1;
	int id = -1;

private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;
	char _szSendBuf[SEND_BUFF_SIZE * 10]{};
	int _lastSendPos = 0;
	time_t _dtHeart;
	time_t _dtSend;
	int _sendBuffFullCount = 0;
};


#endif // !_CELL_CLIENT_HPP_
