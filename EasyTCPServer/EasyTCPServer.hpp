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
#include "MessageHeader.hpp"

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

using namespace std;

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
		SOCKET cSock = INVALID_SOCKET;

		cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == cSock)
		{
			cout << "<socket=" << _sock << ">���󣬽��յ���Ч�Ŀͻ��ˣ�" << endl;
		}
		else
		{
			NewUserJoin userJoin;
			userJoin.sock = cSock;
			SendData2All(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			cout << "<socket=" << _sock << ">�¿ͻ��˼��룺socket = " << cSock << "��"
				<< "IP = " << inet_ntoa(clientAddr.sin_addr) << endl;
		}
		return cSock;
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

		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		timeval t{ 1, 0 };
		SOCKET maxSock = _sock;
		for (auto cSock : _clients)
		{
			FD_SET(cSock->sockfd(), &fdRead);
			maxSock = max(maxSock, cSock->sockfd());
		}
		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
		//cout << "select ret = " << ret << "��count = " << _nCount++ << endl;
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
					delete _clients[i];
					_clients.erase(_clients.begin() + i);
				}
			}
		}
#endif
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
		//cout << "nLen = " << nLen << endl;
		if (nLen <= 0) {
			cout << "<socket=" << _sock << ">���˳�����������" << endl;
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
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//cout << "<socket=" << cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
			//	<< "username=" << login->username << " "
			//	<< "password=" << login->password << endl;

			// ��������
			LoginResult res;
			SendData(cSock, &res);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//cout << "<socket=" << cSock << ">�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
			//	<< "username=" << logout->username << endl;

			// ��������
			LogoutResult res;
			SendData(cSock, &res);
		}
		break;
		default:
		{
			cout << "<socket=" << cSock << ">�յ�δ֪���" << "���ݳ��ȣ�" << header->len << endl;
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

	// ������Ϣ
	void SendData2All(DataHeader* header)
	{
		for (auto cSock : _clients)
		{
			SendData(cSock->sockfd(), header);
		}
	}

private:
	SOCKET _sock;
	vector<ClientSocket*> _clients;
};

#endif // !_EasyTCPServer_hpp_
