#ifndef _EasyTCPServer_hpp_
#define _EasyTCPServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE 1024

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
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

#define _CellServer_THREAD_COUNT 4

// 客户端数据类型
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
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

private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;
};

// 网络事件接口
class INetEvent
{
public:
	// 客户端离开事件
	virtual void OnLeave(ClientSocket* pClient) = 0;
	// 客户端消息事件
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header) = 0;

private:

};


class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;
	}

	~CellServer()
	{
		Close();
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
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
		for (auto cSock : _clients)
		{
			closesocket(cSock->sockfd());
			delete cSock;
		}
		closesocket(_sock);

		// 清理套接字库
		WSACleanup();
#else
		for (auto cSock : _clients)
		{
			close(cSock->sockfd());
			delete cSock;
		}
		close(_sock);
#endif
		_clients.clear();
		_sock = INVALID_SOCKET;
	}

	// 处理网络消息
	//int _nCount = 0;
	bool OnRun()
	{
		while (isRun())
		{
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients.push_back(pClient);
				}
				_clientsBuff.clear();
			}

			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;

			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);

			SOCKET maxSock = _clients[0]->sockfd();
			for (auto cSock : _clients)
			{
				FD_SET(cSock->sockfd(), &fdRead);
				maxSock = max(maxSock, cSock->sockfd());
			}

			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			//std::cout << "select ret = " << ret << "，count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				std::cout << "select任务结束！" << std::endl;
				return false;
			}
#ifdef _WIN32x
			for (unsigned int i = 0; i < fdRead.fd_count; i++)
			{
				if (-1 == RecvData(fdRead.fd_array[i]))
				{
					auto it = find(_clients.begin(), _clients.end(), fdRead.fd_array[i]);
					if (it != _clients.end()) _clients.erase(it);
				}
			}
#else
			for (int i = _clients.size() - 1; i >= 0; i--)
			{
				if (FD_ISSET(_clients[i]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[i]))
					{
						if (_pNetEvent)
							_pNetEvent->OnLeave(_clients[i]);
						delete _clients[i];
						_clients.erase(_clients.begin() + i);
					}
				}
			}
#endif
		}

		return true;
	}

	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	// 缓冲区
	char szRecv[RECV_BUFF_SIZE]{};

	// 接收数据
	int RecvData(ClientSocket* pClient)
	{
		// 接收数据
		int nLen = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE, 0);
		//std::cout << "nLen = " << nLen << std::endl;
		if (nLen <= 0) {
			std::cout << "<socket=" << _sock << ">已退出，结束任务！" << std::endl;
			return -1;
		}
		memcpy(pClient->msgBuf() + pClient->getLast(), szRecv, nLen);
		pClient->setLast(pClient->getLast() + nLen);
		while (pClient->getLast() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			if (pClient->getLast() >= header->len)
			{
				int nSize = pClient->getLast() - header->len;
				OnNetMsg(pClient->sockfd(), header);
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->len, nSize);
				pClient->setLast(nSize);
			}
			else break;
		}

		return 0;
	}

	// 响应网络消息
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		_recvCount++;
		//_pNetEvent->OnNetMsg(cSock, header);

		//auto t1 = _tTime.getElapsedSecond();
		//if (t1 >= 1.0)
		//{
		//	std::cout << "time<" << t1 << ">，socket<" << _sock
		//		<< ">，recvCount<" << _recvCount << ">" << std::endl;
		//	_tTime.update();
		//	_recvCount = 0;
		//}

		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//std::cout << "<socket=" << cSock << ">收到命令：CMD_LOGIN，" << "数据长度：" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			// 发送数据
			LoginResult res;
			SendData(cSock, &res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//std::cout << "<socket=" << cSock << ">收到命令：CMD_LOGIN，" << "数据长度：" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// 发送数据
			LogoutResult res;
			SendData(cSock, &res);
		}
		break;
		default:
		{
			std::cout << "<socket=" << cSock << ">收到未知命令，" << "数据长度：" << header->len << std::endl;
			DataHeader header;
			SendData(cSock, &header);
		}
		break;
		}
	}

	// 发送消息
	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
		_pThread = new std::thread(std::mem_fun(&CellServer::OnRun), this);
	}

	int getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;		// 正式客户队列
	std::vector<ClientSocket*> _clientsBuff;	// 缓冲客户队列
	std::mutex _mutex;
	std::thread* _pThread;
	INetEvent* _pNetEvent;

public:
	std::atomic_int _recvCount;
};


class EasyTCPServer : public INetEvent
{
public:
	EasyTCPServer()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTCPServer()
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
			std::cout << "<socket=" << _sock << ">关闭旧连接" << std::endl;
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			std::cout << "错误，创建套接字失败！" << std::endl;
		}
		std::cout << "<socket=" << _sock << ">创建套接字成功！" << std::endl;
	}

	// 绑定端口
	int Bind(const char* ip, unsigned int port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}

		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip) _sin.sin_addr.S_un.S_addr = inet_addr(ip);
		else _sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		if (ip) _sin.sin_addr.s_addr = inet_addr(ip);
		else _sin.sin_addr.s_addr = INADDR_ANY;
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			std::cout << "错误，绑定端口<" << port << ">失败！" << std::endl;
		}
		else {
			std::cout << "绑定端口<" << port << ">成功！" << std::endl;
		}
		return ret;
	}

	// 监听端口
	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			std::cout << "<socket=" << _sock << ">错误，监听端口失败！" << std::endl;
		}
		else {
			std::cout << "<socket=" << _sock << ">监听端口成功！" << std::endl;
		}
		return ret;
	}

	// 接受连接
	SOCKET Accept()
	{
		// 接受连接
		sockaddr_in clientAddr{};
		int addrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;

		cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == cSock)
		{
			std::cout << "<socket=" << _sock << ">错误，接收到无效的客户端！" << std::endl;
		}
		else
		{
			NewUserJoin userJoin;
			userJoin.sock = cSock;
			SendData2All(&userJoin);

			addClientToCellServer(new ClientSocket(cSock));
			//std::cout << "<socket=" << _sock << ">新客户端加入：socket = " << cSock << "，"
			//	<< "IP = " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		}
		return cSock;
	}

	void addClientToCellServer(ClientSocket* pClient)
	{
		_clients.push_back(pClient);
		auto it = std::min_element(_cellServers.begin(), _cellServers.end(),
			[](CellServer* a, CellServer* b) { return a->getClientCount() < b->getClientCount(); });
		(*it)->addClient(pClient);
	}

	void Start()
	{
		for (int i = 0; i < _CellServer_THREAD_COUNT; i++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			ser->setEventObj(this);
			ser->Start();
		}
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
		for (auto cSock : _clients)
		{
			closesocket(cSock->sockfd());
			delete cSock;
		}
		closesocket(_sock);

		// 清理套接字库
		WSACleanup();
#else
		for (auto cSock : _clients)
		{
			close(cSock->sockfd());
			delete cSock;
		}
		close(_sock);
#endif
		_clients.clear();
		_sock = INVALID_SOCKET;
	}

	// 处理网络消息
	//int _nCount = 0;
	bool OnRun()
	{
		if (!isRun())
		{
			return false;
		}

		time4msg();

		fd_set fdRead;
		//fd_set fdWrite;
		//fd_set fdExp;

		FD_ZERO(&fdRead);
		//FD_ZERO(&fdWrite);
		//FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		//FD_SET(_sock, &fdWrite);
		//FD_SET(_sock, &fdExp);

		timeval t{ 0, 10 };
		int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
		//std::cout << "select ret = " << ret << "，count = " << _nCount++ << std::endl;
		if (ret < 0)
		{
			std::cout << "select任务结束！" << std::endl;
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			// 接受连接
			Accept();
		}
		return true;
	}

	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	// 缓冲区
	char szRecv[RECV_BUFF_SIZE]{};

	// 响应网络消息
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			int recvCount = 0;
			for (auto pCellServer : _cellServers)
			{
				recvCount += pCellServer->_recvCount;
				pCellServer->_recvCount = 0;
			}
			std::cout << "thread<" << _cellServers.size()
				<< ">，time<" << t1 << ">，socket<" << _sock
				<< ">，number<" << _clients.size()
				<< ">，recvCount<" << (int)(recvCount / t1) << ">" << std::endl;
			_tTime.update();
		}
	}

	// 发送消息
	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
	}

	// 发送消息
	void SendData2All(DataHeader* header)
	{
		for (auto cSock : _clients)
		{
			SendData(cSock->sockfd(), header);
		}
	}

	void OnLeave(ClientSocket* pClient)
	{
		for (int i = _clients.size() - 1; i >= 0; i--)
		{
			if (_clients[i] == pClient)
			{
				_clients.erase(_clients.begin() + i);
				return;
			}
		}
	}

	void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		time4msg();
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
	std::vector<CellServer*> _cellServers;
	CELLTimestamp _tTime;
};

#endif // !_EasyTCPServer_hpp_
