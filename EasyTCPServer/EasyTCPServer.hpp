#ifndef _EasyTCPServer_hpp_
#define _EasyTCPServer_hpp_


#include "Cell.hpp"
#include "CellClient.hpp"
#include "CellServer.hpp"
#include "INetEvent.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>


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
			//netmsg_NewUserJoin userJoin;
			//userJoin.sock = cSock;
			//SendData2All(&userJoin);

			addClientToCellServer(new CellClient(cSock));
			//std::cout << "<socket=" << _sock << ">新客户端加入：socket = " << cSock << "，"
			//	<< "IP = " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		}
		return cSock;
	}

	// 将新客户端分配给客户数量最少的cellserver
	void addClientToCellServer(CellClient* pClient)
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
	void SendData2All(netmsg_DataHeader* header)
	{

	}

	// 被一个线程触发，安全
	void OnNetJoin(CellClient* pClient)
	{
		_clientCount++;
	}

	// 被多个线程触发，不安全
	void OnNetLeave(CellClient* pClient)
	{
		_clientCount--;
	}

	// 被多个线程触发，不安全
	void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		_msgCount++;
	}

	// recv事件
	void OnNetRecv(CellClient* pClient)
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
