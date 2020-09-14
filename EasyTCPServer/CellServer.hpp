#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_


#include "Cell.hpp"
#include "INetEvent.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>


// 网络消息接受处理
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
				_oldTime = CELLTime::getNowInMillSec();
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

			timeval t{ 0, 1 };
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
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

			ReadData(fdRead);
			CheckTime();
		}

		return true;
	}

	time_t _oldTime = CELLTime::getNowInMillSec();
	void CheckTime()
	{
		auto nowTime = CELLTime::getNowInMillSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto it = _clients.begin(); it != _clients.end(); )
		{
			if (it->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(it->second);
				closesocket(it->first);
				delete it->second;
				_clients.erase(it++);
				_clients_changed = true;
			}
			else
			{
				it++;
			}
		}
	}

	void ReadData(fd_set& fdRead)
	{
#ifdef _WIN32
		for (int i = 0; i < fdRead.fd_count; i++)
		{
			auto pClient = _clients[fdRead.fd_array[i]];
			if (-1 == RecvData(pClient))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(pClient);
				closesocket(fdRead.fd_array[i]);
				delete pClient;
				_clients.erase(fdRead.fd_array[i]);
				_clients_changed = true;
			}
		}
#else
		for (auto it = _clients.begin(); it != _clients.end(); )
		{
			if (FD_ISSET(it->first, &fdRead) && -1 == RecvData(it->second))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(it->second);
				close(it->first);
				delete it->second;
				_clients.erase(it++);
				_clients_changed = true;
			}
			else
			{
				it++;
			}
		}
#endif	// _WIN32
	}

	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	// 缓冲区
	char szRecv[RECV_BUFF_SIZE]{};

	// 接收数据
	int RecvData(CellClient* pClient)
	{
		// 接收数据
		int nLen = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE, 0);
		_pNetEvent->OnNetRecv(pClient);
		//std::cout << "nLen = " << nLen << std::endl;
		if (nLen <= 0) {
			//std::cout << "<socket=" << _sock << ">已退出，结束任务！" << std::endl;
			return -1;
		}
		memcpy(pClient->msgBuf() + pClient->getLast(), szRecv, nLen);	// 直接用这个接收！！！！！！
		pClient->setLast(pClient->getLast() + nLen);
		while (pClient->getLast() >= sizeof(netmsg_DataHeader))
		{
			netmsg_DataHeader* header = (netmsg_DataHeader*)pClient->msgBuf();
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
	virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	// 发送消息
	int SendData(SOCKET cSock, netmsg_DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
	}

	void addClient(CellClient* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start()
	{
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
		_taskServer.Start();
	}

	int getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

	void addSendTask(CellClient* pClient, netmsg_DataHeader* header)
	{
		_taskServer.addTask([pClient, header]() {
			pClient->SendData(header); });
	}
private:
	SOCKET _sock;
	std::unordered_map<SOCKET, CellClient*> _clients;	// 正式客户队列
	std::vector<CellClient*> _clientsBuff;	// 缓冲客户队列
	std::mutex _mutex;	// 缓冲队列的锁
	std::thread _thread;
	INetEvent* _pNetEvent;	// 网络事件对象
	CellTaskServer _taskServer;
};


#endif // !_CELL_SERVER_HPP_
