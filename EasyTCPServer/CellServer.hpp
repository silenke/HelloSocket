#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_


#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CELLSemaphore.hpp"

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
	CellServer(int id)
	{
		_id = id;
		_pNetEvent = nullptr;
		_taskServer.serverId = _id;
	}

	~CellServer()
	{
		std::cout << "\tCellServer<" << _id
			<< ">.~CellServer exit begin" << std::endl;
		Close();
		std::cout << "\tCellServer<" << _id
			<< ">.~CellServer exit end" << std::endl;
	}

	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}

	// 关闭套接字
	void Close()
	{
		std::cout << "\t\tCellServer<" << _id
			<< ">.Close exit begin" << std::endl;
		if (_bRun)
		{
			_taskServer.Close();
			_bRun = false;
			_sem.wait();
		}
		std::cout << "\t\tCellServer<" << _id
			<< ">.Close exit end" << std::endl;
	}

	// 处理网络消息
	void OnRun()
	{
		while (_bRun)
		{
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
					pClient->serverId = _id;
					//if (_pNetEvent)
					//	_pNetEvent->OnNetJoin(pClient);
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
				return;
			}
			//else if (ret == 0)
			//{
			//	continue;
			//}

			ReadData(fdRead);
			CheckTime();
		}

		std::cout << "\t\t\tCellServer<" << _id
			<< ">.OnRun exit" << std::endl;

		ClearClients();
		_sem.wakeup();
	}

	void CheckTime()
	{
		auto nowTime = CELLTime::getNowInMillSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto it = _clients.begin(); it != _clients.end(); )
		{
			// 心跳检测
			if (it->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(it->second);
				delete it->second;
				_clients.erase(it++);
				_clients_changed = true;
			}
			else
			{	// 定时发送检测
				it->second->checkSend(dt);
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
		if (_bRun && header)
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
		if (_bRun) return;

		_bRun = true;
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
		_thread.detach();
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
	void ClearClients()
	{
		for (auto p : _clients)
		{
			delete p.second;
		}
		_clients.clear();
		for (auto p : _clientsBuff)
		{
			delete p;
		}
		_clientsBuff.clear();
	}

private:
	SOCKET _sock;
	std::unordered_map<SOCKET, CellClient*> _clients;	// 正式客户队列
	std::vector<CellClient*> _clientsBuff;	// 缓冲客户队列
	std::mutex _mutex;	// 缓冲队列的锁
	std::thread _thread;
	INetEvent* _pNetEvent;	// 网络事件对象
	CellTaskServer _taskServer;
	fd_set _fdRead_bak;
	SOCKET _maxSock;
	time_t _oldTime = CELLTime::getNowInMillSec();
	int _id = -1;
	bool _clients_changed = true;
	bool _bRun = false;
	CELLSemaphore _sem;
};


#endif // !_CELL_SERVER_HPP_
