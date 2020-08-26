#ifndef _EasyTCPServer_hpp_
#define _EasyTCPServer_hpp_

#ifdef _WIN32
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
#include "MessageHeader.hpp"

using namespace std;

class EasyTCPServer
{
public:
	EasyTCPServer()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTCPServer()
	{

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
			cout << "<socket=" << _sock << ">�رվ�����" << endl;
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			cout << "���󣬴����׽���ʧ�ܣ�" << endl;
		}
		cout << "<socket=" << _sock << ">�����׽��ֳɹ���" << endl;
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
		_sin.sin_port = htons(6100);
#ifdef _WIN32
		if (ip) _sin.sin_addr.S_un.S_addr = inet_addr(ip);
		else _sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		if (ip) _sin.sin_addr.s_addr = inet_addr(ip);
		else _sin.sin_addr.s_addr = INADDR_ANY;
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret) {
			cout << "���󣬰󶨶˿�<" << port << ">ʧ�ܣ�" << endl;
		}
		else {
			cout << "�󶨶˿�<" << port << ">�ɹ���" << endl;
		}
		return ret;
	}

	// �����˿�
	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			cout << "<socket=" << _sock << ">���󣬼����˿�ʧ�ܣ�" << endl;
		}
		else {
			cout << "<socket=" << _sock << ">�����˿ڳɹ���" << endl;
		}
		return ret;
	}

	// ��������
	SOCKET Accept()
	{
		// ��������
		sockaddr_in clientAddr{};
		int addrLen = sizeof(sockaddr_in);
		SOCKET _cSock = INVALID_SOCKET;

		_cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == _cSock)
		{
			cout << "<socket=" << _sock << ">���󣬽��յ���Ч�Ŀͻ��ˣ�" << endl;
		}
		else
		{
			NewUserJoin userJoin;
			userJoin.sock = _cSock;
			SendData2All(&userJoin);
			g_clients.push_back(_cSock);
			cout << "<socket=" << _sock << ">�¿ͻ��˼��룺socket = " << _cSock << "��"
				<< "IP = " << inet_ntoa(clientAddr.sin_addr) << endl;
		}
		return _cSock;
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
		_sock = INVALID_SOCKET;
	}

	// ����������Ϣ
	bool OnRun()
	{
		if (!isRun())
		{
			return false;
		}

		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (auto _csock : g_clients) {
			FD_SET(_csock, &fdRead);
		}

		timeval t{};
		SOCKET maxSock = _sock;
		for (auto _cSock : g_clients)
		{
			maxSock = max(maxSock, _cSock);
		}
		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			cout << "select���������" << endl;
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			// ��������
			Accept();
		}
#ifdef _WIN32
		for (unsigned int i = 0; i < fdRead.fd_count; i++)
		{
			if (-1 == RecvData(fdRead.fd_array[i]))
			{
				auto it = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[i]);
				if (it != g_clients.end()) g_clients.erase(it);
			}
		}
#else
		for (int i = g_clients.size() - 1; i >= 0; i--)
		{
			if (FD_ISSET(g_clients[i], &fdRead) && -1 == processor(g_clients[i]))
			{
				g_clients.erase(g_clients.begin() + i);
			}
		}
#endif
		return true;
	}

	//
	bool isRun()
	{
		return INVALID_SOCKET != _sock;
	}

	// ��������
	int RecvData(SOCKET _cSock)
	{
		// ��������
		char szRecv[1024]{};
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		if (nLen <= 0) {
			cout << "<socket=" << _cSock << ">�ͻ������˳�����������" << endl;
			return -1;
		}
		// if (nLen >= sizeof(DataHeader)) �����ٰ�
		DataHeader* header = (DataHeader*)szRecv;

		// ��������
		cout << "<socket=" << _cSock << ">�յ����" << header->cmd << " "
			<< "���ݳ��ȣ�" << header->len << endl;

		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		OnNetMsg(_cSock, header);

		return 0;
	}

	// ��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET _cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			cout << "<socket=" << _cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
				<< "username=" << login->username << " "
				<< "password=" << login->password << endl;

			// ��������
			LoginResult res;
			SendData(_cSock, &res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			cout << "<socket=" << _cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
				<< "username=" << logout->username << endl;

			// ��������
			LogoutResult res;
			SendData(_cSock, &res);
		}
		break;
		default:
		{
			header->cmd = CMD_ERROR;
			header->len = 0;
			SendData(_cSock, header);
		}
		break;
		}
	}

	// ������Ϣ
	int SendData(SOCKET _cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_cSock, (const char*)header, header->len, 0);
		}
		return SOCKET_ERROR;
	}

	// ������Ϣ
	void SendData2All(DataHeader* header)
	{
		for (auto _cSock : g_clients)
		{
			SendData(_cSock, header);
		}
	}

private:
	SOCKET _sock;
	vector<SOCKET> g_clients;
};

#endif // !_EasyTCPServer_hpp_
