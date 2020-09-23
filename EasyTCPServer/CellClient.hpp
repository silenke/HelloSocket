#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_


#include "Cell.hpp"
#include "CELLBuffer.hpp"

#include <iostream>


#define CLIENT_HEART_DEAD_TIME 60000
#define CLIENT_SEND_BUFF_TIME 200


// 客户端数据类型
class CellClient
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET) :
		_recvBuff(RECV_BUFF_SIZE),
		_sendBuff(SEND_BUFF_SIZE)
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

	int recvData()
	{
		return _recvBuff.read4socket(_sockfd);
	}

	bool hasMsg()
	{
		return _recvBuff.hasMsg();
	}

	netmsg_DataHeader* front_msg()
	{
		return (netmsg_DataHeader*)_recvBuff.data();
	}

	void pop_front_msg()
	{
		if (hasMsg())
		{
			_recvBuff.pop(front_msg()->len);
		}
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
		resetDTSend();
		return _sendBuff.write2socket(_sockfd);
	}

	// 发送消息
	int SendData(netmsg_DataHeader* header)
	{
		if (_sendBuff.push((const char*)header, header->len))
		{
			return header->len;
		}
		return SOCKET_ERROR;
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
	CELLBuffer _recvBuff;
	CELLBuffer _sendBuff;
	time_t _dtHeart;
	time_t _dtSend;
	int _sendBuffFullCount = 0;
};


#endif // !_CELL_CLIENT_HPP_
