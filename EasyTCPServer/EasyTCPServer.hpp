#ifndef _EasyTCPServer_hpp_
#define _EasyTCPServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE 2048

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
#include <unordered_map>
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

	// 发送消息
	int SendData(DataHeader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
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
	// 客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	// 客户端离开事件
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	// 客户端消息事件
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;
	// recv事件
	virtual void OnNetRecv(ClientSocket* pClient) = 0;

private:

};


class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
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
		for (auto p : _clients)
		{
			closesocket(p.first);
			delete p.second;
		}
#else
		for (auto p : _clients)
		{
			close(p.first);
			delete p.second;
		}
#endif
		_clients.clear();
	}

	// 处理网络消息
	fd_set _fdRead_bak;
	bool _clients_changed;
	SOCKET _maxSock;
	bool OnRun()
	{
		_clients_changed = true;
		while (isRun())
		{
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_changed = true;
			}

			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			fd_set fdRead;

			FD_ZERO(&fdRead);

			if (_clients_changed)
			{
				_clients_changed = false;
				_maxSock = _clients.begin()->first;
				for (auto p : _clients)
				{
					FD_SET(p.first, &fdRead);
					_maxSock = max(_maxSock, p.first);
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else
			{
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			//timeval t{ 0, 0 };
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr,nullptr);
			//std::cout << "select ret = " << ret << "，count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				std::cout << "select任务结束！" << std::endl;
				return false;
			}
			//else if (ret == 0)
			//{
			//	continue;
			//}

#ifdef _WIN32
			for (int i = 0; i < fdRead.fd_count; i++)
			{
				auto pClient = _clients[fdRead.fd_array[i]];
				if (-1 == RecvData(pClient))
				{
					if (_pNetEvent)
						_pNetEvent->OnNetLeave(pClient);
					delete pClient;
					_clients.erase(fdRead.fd_array[i]);
					_clients_changed = true;
				}
			}
#else
			for (auto p : _clients)
			{
				if (FD_ISSET(p.first, &fdRead))
				{
					if (-1 == RecvData(p.second))
					{
						if (_pNetEvent)
							_pNetEvent->OnNetLeave(p.second);
						delete p.second;
						_clients.erase(p.first);
						_clients_changed = true;
					}
				}
			}
#endif	// _WIN32
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
		_pNetEvent->OnNetRecv(pClient);
		//std::cout << "nLen = " << nLen << std::endl;
		if (nLen <= 0) {
			//std::cout << "<socket=" << _sock << ">已退出，结束任务！" << std::endl;
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
				OnNetMsg(pClient, header);
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->len, nSize);
				pClient->setLast(nSize);
			}
			else break;
		}

		return 0;
	}

	// 响应网络消息
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(pClient, header);
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
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}

	int getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

private:
	SOCKET _sock;
	std::unordered_map<SOCKET, ClientSocket*> _clients;	// 正式客户队列
	std::vector<ClientSocket*> _clientsBuff;	// 缓冲客户队列
	std::mutex _mutex;	// 缓冲队列的锁
	std::thread _thread;
	INetEvent* _pNetEvent;	// 网络事件对象
};


class EasyTCPServer : public INetEvent
{
public:
	EasyTCPServer()
	{
		_sock = INVALID_SOCKET;
		_msgCount = 0;
		_recvCount = 0;
		_clientCount = 0;
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
			//NewUserJoin userJoin;
			//userJoin.sock = cSock;
			//SendData2All(&userJoin);

			addClientToCellServer(new ClientSocket(cSock));
			//std::cout << "<socket=" << _sock << ">新客户端加入：socket = " << cSock << "，"
			//	<< "IP = " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		}
		return cSock;
	}

	// 将新客户端分配给客户数量最少的cellserver
	void addClientToCellServer(ClientSocket* pClient)
	{
		auto it = std::min_element(_cellServers.begin(), _cellServers.end(),
			[](CellServer* a, CellServer* b) { return a->getClientCount() < b->getClientCount(); });
		(*it)->addClient(pClient);
		OnNetJoin(pClient);
	}

	void Start(int nCellServer)
	{
		for (int i = 0; i < nCellServer; i++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			ser->setEventObj(this);	// 注册网络事件接收对象
			ser->Start();	// 启动消息处理线程
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
		closesocket(_sock);

		// 清理套接字库
		WSACleanup();
#else
		close(_sock);
#endif
		// _clients.clear();
		_sock = INVALID_SOCKET;
	}

	// 处理网络消息
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
			std::cout << "accept select任务结束！" << std::endl;
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

	// 计数输出每秒收到的网络消息
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			std::cout << "thread<" << _cellServers.size()
				<< ">，time<" << t1
				<< ">，socket<" << _sock
				<< ">，clients<" << _clientCount
				<< ">，recv<" << (int)(_recvCount / t1)
				<< ">，msg<" << (int)(_msgCount / t1)
				<< ">" << std::endl;
			_msgCount = 0;
			_recvCount = 0;
			_tTime.update();
		}
	}

	// 发送消息
	void SendData2All(DataHeader* header)
	{

	}

	// 被一个线程触发，安全
	void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}

	// 被多个线程触发，不安全
	void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}

	// 被多个线程触发，不安全
	void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
	}

private:
	SOCKET _sock;
	std::vector<CellServer*> _cellServers;	// 处理消息对象，内部会创建线程
	CELLTimestamp _tTime;	// 每秒消息计时

protected:
	std::atomic_int _msgCount;	// 收到消息计数
	std::atomic_int _clientCount;	// 客户端加入计数
	std::atomic_int _recvCount;	// 收到recv计数
};

#endif // !_EasyTCPServer_hpp_
