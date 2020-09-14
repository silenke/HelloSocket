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
			//netmsg_NewUserJoin userJoin;
			//userJoin.sock = cSock;
			//SendData2All(&userJoin);

			addClientToCellServer(new CellClient(cSock));
			//std::cout << "<socket=" << _sock << ">�¿ͻ��˼��룺socket = " << cSock << "��"
			//	<< "IP = " << inet_ntoa(clientAddr.sin_addr) << std::endl;
		}
		return cSock;
	}

	// ���¿ͻ��˷�����ͻ��������ٵ�cellserver
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
	void SendData2All(netmsg_DataHeader* header)
	{

	}

	// ��һ���̴߳�������ȫ
	void OnNetJoin(CellClient* pClient)
	{
		_clientCount++;
	}

	// ������̴߳���������ȫ
	void OnNetLeave(CellClient* pClient)
	{
		_clientCount--;
	}

	// ������̴߳���������ȫ
	void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header)
	{
		_msgCount++;
	}

	// recv�¼�
	void OnNetRecv(CellClient* pClient)
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
