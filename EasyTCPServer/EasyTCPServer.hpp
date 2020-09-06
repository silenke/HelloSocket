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

// �ͻ�����������
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

	// ������Ϣ
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

// �����¼��ӿ�
class INetEvent
{
public:
	// �ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	// �ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	// �ͻ�����Ϣ�¼�
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header) = 0;
	// recv�¼�
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

	// �ر��׽���
	void Close()
	{
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

#ifdef _WIN32
		// �ر��׽���
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

	// ����������Ϣ
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
			//std::cout << "select ret = " << ret << "��count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				std::cout << "select���������" << std::endl;
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

	// ������
	char szRecv[RECV_BUFF_SIZE]{};

	// ��������
	int RecvData(ClientSocket* pClient)
	{
		// ��������
		int nLen = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE, 0);
		_pNetEvent->OnNetRecv(pClient);
		//std::cout << "nLen = " << nLen << std::endl;
		if (nLen <= 0) {
			//std::cout << "<socket=" << _sock << ">���˳�����������" << std::endl;
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

	// ��Ӧ������Ϣ
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(pClient, header);
	}

	// ������Ϣ
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
	std::unordered_map<SOCKET, ClientSocket*> _clients;	// ��ʽ�ͻ�����
	std::vector<ClientSocket*> _clientsBuff;	// ����ͻ�����
	std::mutex _mutex;	// ������е���
	std::thread _thread;
	INetEvent* _pNetEvent;	// �����¼�����
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

	// ��ʼ���׽���
	void InitSocket()
	{
#ifdef _WIN32
		// ��ʼ���׽��ֿ�
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		WSAStartup(version, &data);
#endif
		// �����׽���
		if (INVALID_SOCKET != _sock)
		{
			Close();
			std::cout << "<socket=" << _sock << ">�رվ�����" << std::endl;
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			std::cout << "���󣬴����׽���ʧ�ܣ�" << std::endl;
		}
		std::cout << "<socket=" << _sock << ">�����׽��ֳɹ���" << std::endl;
	}

	// �󶨶˿�
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
			std::cout << "���󣬰󶨶˿�<" << port << ">ʧ�ܣ�" << std::endl;
		}
		else {
			std::cout << "�󶨶˿�<" << port << ">�ɹ���" << std::endl;
		}
		return ret;
	}

	// �����˿�
	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			std::cout << "<socket=" << _sock << ">���󣬼����˿�ʧ�ܣ�" << std::endl;
		}
		else {
			std::cout << "<socket=" << _sock << ">�����˿ڳɹ���" << std::endl;
		}
		return ret;
	}

	// ��������
	SOCKET Accept()
	{
		// ��������
		sockaddr_in clientAddr{};
		int addrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;

		cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == cSock)
		{
			std::cout << "<socket=" << _sock << ">���󣬽��յ���Ч�Ŀͻ��ˣ�" << std::endl;
		}
		else
		{
			//NewUserJoin userJoin;
			//userJoin.sock = cSock;
			//SendData2All(&userJoin);

			addClientToCellServer(new ClientSocket(cSock));
			//std::cout << "<socket=" << _sock << ">�¿ͻ��˼��룺socket = " << cSock << "��"
			//	<< "IP = " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		}
		return cSock;
	}

	// ���¿ͻ��˷�����ͻ��������ٵ�cellserver
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
			ser->setEventObj(this);	// ע�������¼����ն���
			ser->Start();	// ������Ϣ�����߳�
		}
	}

	// �ر��׽���
	void Close()
	{
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

#ifdef _WIN32
		// �ر��׽���
		closesocket(_sock);

		// �����׽��ֿ�
		WSACleanup();
#else
		close(_sock);
#endif
		// _clients.clear();
		_sock = INVALID_SOCKET;
	}

	// ����������Ϣ
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
		//std::cout << "select ret = " << ret << "��count = " << _nCount++ << std::endl;
		if (ret < 0)
		{
			std::cout << "accept select���������" << std::endl;
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			// ��������
			Accept();
		}
		return true;
	}

	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	// �������ÿ���յ���������Ϣ
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			std::cout << "thread<" << _cellServers.size()
				<< ">��time<" << t1
				<< ">��socket<" << _sock
				<< ">��clients<" << _clientCount
				<< ">��recv<" << (int)(_recvCount / t1)
				<< ">��msg<" << (int)(_msgCount / t1)
				<< ">" << std::endl;
			_msgCount = 0;
			_recvCount = 0;
			_tTime.update();
		}
	}

	// ������Ϣ
	void SendData2All(DataHeader* header)
	{

	}

	// ��һ���̴߳�������ȫ
	void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}

	// ������̴߳���������ȫ
	void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}

	// ������̴߳���������ȫ
	void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_recvCount++;
	}

private:
	SOCKET _sock;
	std::vector<CellServer*> _cellServers;	// ������Ϣ�����ڲ��ᴴ���߳�
	CELLTimestamp _tTime;	// ÿ����Ϣ��ʱ

protected:
	std::atomic_int _msgCount;	// �յ���Ϣ����
	std::atomic_int _clientCount;	// �ͻ��˼������
	std::atomic_int _recvCount;	// �յ�recv����
};

#endif // !_EasyTCPServer_hpp_
