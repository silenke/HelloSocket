#ifndef _EasyTCPServer_hpp_
#define _EasyTCPServer_hpp_


#include "Cell.hpp"
#include "CellClient.hpp"
#include "CellServer.hpp"
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
		CELLNetWork::Init();
		// �����׽���
		if (INVALID_SOCKET != _sock)
		{
			Close();
			CELLlog::Info("<socket=%lld>�رվ�����\n", _sock);
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			CELLlog::Info("���󣬴����׽���ʧ�ܣ�\n");
		}
		CELLlog::Info("<socket=%lld>�����׽��ֳɹ���\n", _sock);
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
			CELLlog::Info("���󣬰󶨶˿�<%d>ʧ�ܣ�\n", port);
		}
		else {
			CELLlog::Info("�󶨶˿�<%d>�ɹ���\n", port);
		}
		return ret;
	}

	// �����˿�
	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			CELLlog::Info("<socket=%d>���󣬼����˿�ʧ�ܣ�\n", _sock);
		}
		else {
			CELLlog::Info("<socket=%d>�����˿ڳɹ���\n", _sock);
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
			CELLlog::Info("<socket=%d>���󣬽��յ���Ч�Ŀͻ��ˣ�\n", _sock);
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
			auto ser = new CellServer(i);
			_cellServers.push_back(ser);
			ser->setEventObj(this);	// ע�������¼����ն���
			ser->Start();	// ������Ϣ�����߳�
		}
		_thread.Start(nullptr, [this](CellThread* pThread) {OnRun(pThread); });
	}

	// �ر��׽���
	void Close()
	{
		CELLlog::Info("EasyTCPServer.Close begin\n");
		//
		_thread.Close();
		if (INVALID_SOCKET == _sock)
		{
			return;
		}

		for (auto s : _cellServers)
		{
			delete s;
		}
		_cellServers.clear();

#ifdef _WIN32
		// �ر��׽���
		closesocket(_sock);
#else
		close(_sock);
#endif
		// _clients.clear();
		_sock = INVALID_SOCKET;
		//
		CELLlog::Info("EasyTCPServer.Close end\n");
	}

	// �������ÿ���յ���������Ϣ
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			CELLlog::Info("thread<%lld>��time<%lld>��socket<%d>��clients<%d>��recv<%d>��msg<%d>\n",
				_cellServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
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
	// ����������Ϣ
	void OnRun(CellThread* pThread)
	{
		while (pThread->isRun())
		{
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

			timeval t{ 0, 1 };
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			//std::cout << "select ret = " << ret << "��count = " << _nCount++ << std::endl;
			if (ret < 0)
			{
				CELLlog::Info("EasyTCPServer.OnRun select error��\n");
				pThread->Exit();
				break;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				// ��������
				Accept();
			}
		}
	}

private:
	SOCKET _sock;
	std::vector<CellServer*> _cellServers;	// ������Ϣ�����ڲ��ᴴ���߳�
	CELLTimestamp _tTime;	// ÿ����Ϣ��ʱ
	CellThread _thread;

protected:
	std::atomic_int _msgCount;	// �յ���Ϣ����
	std::atomic_int _clientCount;	// �ͻ��˼������
	std::atomic_int _recvCount;	// �յ�recv����
};


#endif // !_EasyTCPServer_hpp_
