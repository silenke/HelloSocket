#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_


#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CellThread.hpp"

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
			_taskServer.Close();
			_thread.Close();
		std::cout << "\t\tCellServer<" << _id
			<< ">.Close exit end" << std::endl;
	}

	// 处理网络消息
	void OnRun(CellThread* pThread)
	{
		while (pThread->isRun())
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
			fd_set fdWrite;
			//fd_set fdExc;

			if (_clients_changed)
			{
				_clients_changed = false;
				FD_ZERO(&fdRead);
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

			memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
			//memcpy(&fdExc, &_fdRead_bak, sizeof(fd_set));

			timeval t{ 0, 1 };
			int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			//std::cout << "select ret = " << ret << "，count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				std::cout << "CellServer<" << _id
					<< ">.OnRun.select Error！" << std::endl;
				pThread->Exit();
				break;
			}
			//else if (ret == 0)
			//{
			//	continue;
			//}

			ReadData(fdRead);
			WriteData(fdWrite);
			//WriteData(fdExc);

			//std::cout << "CellServer<" << _id
			//	<< ">.OnRun.select：fdRead=" << fdRead.fd_count
			//	<< "，fdWrite=" << fdWrite.fd_count << std::endl;
			//if (fdExc.fd_count > 0)
			//{
			//	std::cout << "CellServer<" << _id
			//		<< "，fdExc=" << fdExc.fd_count << std::endl;
			//}

			CheckTime();
		}

		std::cout << "\t\t\tCellServer<" << _id
			<< ">.OnRun exit" << std::endl;
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
			//else
			//{	// 定时发送检测
			//	it->second->checkSend(dt);
				it++;
			//}
		}
	}

	void OnClientLeave(CellClient* pClient)
	{
		if (_pNetEvent)
			_pNetEvent->OnNetLeave(pClient);
		delete pClient;
		_clients_changed = true;
	}

	void WriteData(fd_set& fdWrite)
	{
#ifdef _WIN32
		for (int i = 0; i < fdWrite.fd_count; i++)
		{
			auto pClient = _clients[fdWrite.fd_array[i]];
			if (-1 == pClient->SendDataReal())
			{
				OnClientLeave(pClient);
				_clients.erase(fdWrite.fd_array[i]);
			}
		}
#else
		for (auto it = _clients.begin(); it != _clients.end(); )
		{
			if (FD_ISSET(it->first, &fdWrite) && -1 == it->second->SendDataReal())
			{
				OnClientLeave(it->second);
				_clients.erase(it++);
			}
			else
			{
				it++;
			}
		}
#endif	// _WIN32
	}

	void ReadData(fd_set& fdRead)
	{
#ifdef _WIN32
		for (int i = 0; i < fdRead.fd_count; i++)
		{
			auto pClient = _clients[fdRead.fd_array[i]];
			if (-1 == RecvData(pClient))
			{
				OnClientLeave(pClient);
				_clients.erase(fdRead.fd_array[i]);
			}
		}
#else
		for (auto it = _clients.begin(); it != _clients.end(); )
		{
			if (FD_ISSET(it->first, &fdRead) && -1 == RecvData(it->second))
			{
				OnClientLeave(it->second);
				_clients.erase(it++);
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
		int nLen = pClient->recvData();
		if (nLen < 0)
		{
			return -1;
		}
		_pNetEvent->OnNetRecv(pClient);
		while (pClient->hasMsg())
		{
			OnNetMsg(pClient, pClient->front_msg());
			pClient->pop_front_msg();
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
		_taskServer.Start();
		_thread.Start(nullptr,
			[this](CellThread* pThread) {OnRun(pThread); },
			[this](CellThread* pThread) {ClearClients(); });
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
	INetEvent* _pNetEvent;	// 网络事件对象
	CellTaskServer _taskServer;
	fd_set _fdRead_bak;
	SOCKET _maxSock;
	time_t _oldTime = CELLTime::getNowInMillSec();
	int _id = -1;
	bool _clients_changed = true;
	bool _bRun = false;
	CellThread _thread;
};


#endif // !_CELL_SERVER_HPP_
