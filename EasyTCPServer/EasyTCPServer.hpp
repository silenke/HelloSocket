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

private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;
};

// �����¼��ӿ�
class INetEvent
{
public:
	// �ͻ����뿪�¼�
	virtual void OnLeave(ClientSocket* pClient) = 0;
	// �ͻ�����Ϣ�¼�
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

	// �ر��׽���
	void Close()
	{
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

#ifdef _WIN32
		// �ر��׽���
		for (auto cSock : _clients)
		{
			closesocket(cSock->sockfd());
			delete cSock;
		}
		closesocket(_sock);

		// �����׽��ֿ�
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

	// ����������Ϣ
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
			//std::cout << "select ret = " << ret << "��count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				std::cout << "select���������" << std::endl;
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

	// ������
	char szRecv[RECV_BUFF_SIZE]{};

	// ��������
	int RecvData(ClientSocket* pClient)
	{
		// ��������
		int nLen = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE, 0);
		//std::cout << "nLen = " << nLen << std::endl;
		if (nLen <= 0) {
			std::cout << "<socket=" << _sock << ">���˳�����������" << std::endl;
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

	// ��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		_recvCount++;
		//_pNetEvent->OnNetMsg(cSock, header);

		//auto t1 = _tTime.getElapsedSecond();
		//if (t1 >= 1.0)
		//{
		//	std::cout << "time<" << t1 << ">��socket<" << _sock
		//		<< ">��recvCount<" << _recvCount << ">" << std::endl;
		//	_tTime.update();
		//	_recvCount = 0;
		//}

		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//std::cout << "<socket=" << cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << std::endl;

			// ��������
			LoginResult res;
			SendData(cSock, &res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//std::cout << "<socket=" << cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << std::endl
			//	<< "username=" << logout->username << std::endl;

			// ��������
			LogoutResult res;
			SendData(cSock, &res);
		}
		break;
		default:
		{
			std::cout << "<socket=" << cSock << ">�յ�δ֪���" << "���ݳ��ȣ�" << header->len << std::endl;
			DataHeader header;
			SendData(cSock, &header);
		}
		break;
		}
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
		_pThread = new std::thread(std::mem_fun(&CellServer::OnRun), this);
	}

	int getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;		// ��ʽ�ͻ�����
	std::vector<ClientSocket*> _clientsBuff;	// ����ͻ�����
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
			NewUserJoin userJoin;
			userJoin.sock = cSock;
			SendData2All(&userJoin);

			addClientToCellServer(new ClientSocket(cSock));
			//std::cout << "<socket=" << _sock << ">�¿ͻ��˼��룺socket = " << cSock << "��"
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

	// �ر��׽���
	void Close()
	{
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

#ifdef _WIN32
		// �ر��׽���
		for (auto cSock : _clients)
		{
			closesocket(cSock->sockfd());
			delete cSock;
		}
		closesocket(_sock);

		// �����׽��ֿ�
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

	// ����������Ϣ
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
		//std::cout << "select ret = " << ret << "��count = " << _nCount++ << std::endl;
		if (ret < 0)
		{
			std::cout << "select���������" << std::endl;
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

	// ������
	char szRecv[RECV_BUFF_SIZE]{};

	// ��Ӧ������Ϣ
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
				<< ">��time<" << t1 << ">��socket<" << _sock
				<< ">��number<" << _clients.size()
				<< ">��recvCount<" << (int)(recvCount / t1) << ">" << std::endl;
			_tTime.update();
		}
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

	// ������Ϣ
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
