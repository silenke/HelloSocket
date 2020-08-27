#ifndef _EasyTCPClient_hpp_
#define _EasyTCPClient_hpp_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS

	#include <Windows.h>
	#include <WinSock2.h>

	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOKCET unsigned long long
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <iostream>
#include "MessageHeader.hpp"

using namespace std;

class EasyTCPClient
{
public:
	EasyTCPClient()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTCPClient()
	{
		Close();
	}

	// 初始化套接字
	void InitSocket()
	{
#ifdef _WIN32
		// 初始化套接字库
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(version, &data);
#endif
		// 创建套接字
		if (INVALID_SOCKET != _sock)
		{
			Close();
			cout << "<socket=" << _sock << ">关闭旧连接" << endl;
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			cout << "错误，创建套接字失败！" << endl;
		}
		cout << "<socket=" << _sock << ">创建套接字成功！" << endl;
	}

	// 连接服务器
	int Connect(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}

		// 发起连接
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			cout << "<socket=" << _sock << ">错误，连接服务器<"
				<< ip << ":" << port << ">失败！" << endl;
		}
		cout << "<socket=" << _sock << ">连接服务器<"
			<< ip << ":" << port << ">成功！" << endl;

		return ret;
	}

	// 关闭套接字
	void Close()
	{
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

#ifdef _WIN32
		// 关闭套接字
		closesocket(_sock);

		// 清理套接字库
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}

	// 查询网络消息
	//int _nCount = 0;
	bool OnRun()
	{
		if (!isRun())
		{
			return false;
		}

		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval t{};
		int ret = select(_sock + 1, &fdRead, NULL, NULL, &t);
		//cout << "select ret = " << ret << "，count = " << _nCount++ << endl;
		if (ret < 0)
		{
			cout << "<socket=" << _sock << ">select任务结束1！" << endl;
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == RecvData())
			{
				cout << "<socket=" << _sock << ">select任务结束2！" << endl;
				Close();
				return false;
			}
		}
		return true;
	}

	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE
	// 缓冲区
	char _szRecv[RECV_BUFF_SIZE]{};
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;

	// 接收数据
	int RecvData()
	{
		// 接收数据
		int nLen = recv(_sock, _szRecv, RECV_BUFF_SIZE, 0);
		//cout << "nLen = " << nLen <<  endl;
		if (nLen <= 0) {
			cout << "<socket=" << _sock << ">与服务器断开连接，结束任务！" << endl;
			return -1;
		}
		memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		_lastPos += nLen;
		while (_lastPos >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)_szMsgBuf;
			if (_lastPos >= header->len)
			{
				int nSize = _lastPos - header->len;
				OnNetMsg(header);
				memcpy(_szMsgBuf, _szMsgBuf + header->len, nSize);
				_lastPos = nSize;
			}
			else break;
		}

		return 0;
	}

	// 响应网络消息
	virtual void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			// 接收数据
			LoginResult* login = (LoginResult*)header;
			//cout << "<socket=" << _sock << ">收到命令：CMD_LOGIN_RESULT，"
			//	<< "数据长度：" << header->len << endl;
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			// 接收数据
			LogoutResult* logout = (LogoutResult*)header;
			//cout << "<socket=" << _sock << ">收到命令：CMD_LOGOUT_RESULT，"
			//	<< "数据长度：" << header->len << endl;
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			// 接收数据
			NewUserJoin* userJoin = (NewUserJoin*)header;
			//cout << "<socket=" << _sock << ">收到命令：CMD_NEW_USER_JOIN，"
			//	<< "数据长度：" << header->len << endl;
		}
		break;
		case CMD_ERROR:
		{
			//cout << "<socket=" << _sock << ">收到命令：CMD_ERROR，"
			//	<< "数据长度：" << header->len << endl;
		}
		break;
		default:
		{
			cout << "<socket=" << _sock << ">收到未知命令，"
				<< "数据长度：" << header->len << endl;
		}
		break;
		}
	}

	// 发送消息
	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
	}

private:
	SOCKET _sock;
};

#endif // _EasyTCPClient_hpp_
