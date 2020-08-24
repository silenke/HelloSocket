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

using namespace std;

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

struct DataHeader
{
	short cmd;
	short len;
};

struct Login : public DataHeader
{
	Login() {
		cmd = CMD_LOGIN;
		len = sizeof(Login);
	}
	char username[32];
	char password[32];
};

struct LoginResult : public DataHeader
{
	LoginResult() {
		cmd = CMD_LOGIN_RESULT;
		len = sizeof(LoginResult);
		result = 0;
	}
	int result;
};

struct Logout : public DataHeader
{
	Logout() {
		cmd = CMD_LOGOUT;
		len = sizeof(Logout);
	}
	char username[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult() {
		cmd = CMD_LOGOUT_RESULT;
		len = sizeof(LogoutResult);
		result = 0;
	}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin() {
		cmd = CMD_NEW_USER_JOIN;
		len = sizeof(NewUserJoin);
	}
	SOCKET sock;
};

vector<SOCKET> g_clients;

int processor(SOCKET _cSock)
{
	// ��������
	char szRecv[1024]{};
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0) {
		cout << "�ͻ��ˣ�" << _cSock << "�����˳�����������" << endl;
		return -1;
	}
	// if (nLen >= sizeof(DataHeader)) �����ٰ�
	DataHeader* header = (DataHeader*)szRecv;

	// ��������
	cout << "�յ����" << header->cmd << " "
		<< "���ݳ��ȣ�" << header->len << endl;

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		// ��������
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		Login* login = (Login*)szRecv;
		cout << "�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
			<< "username=" << login->username << " "
			<< "password=" << login->password << endl;

		// ��������
		LoginResult res;
		send(_cSock, (const char*)&res, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		// ��������
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		Logout* logout = (Logout*)szRecv;
		cout << "�յ����CMD_LOGIN��" << "���ݳ��ȣ�" << header->len << endl
			<< "username=" << logout->username << endl;

		// ��������
		LogoutResult res;
		send(_cSock, (const char*)&res, sizeof(LogoutResult), 0);
	}
	break;
	default:
	{
		header->cmd = CMD_ERROR;
		header->len = 0;
		send(_cSock, (const char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}

	return 0;
}

int main()
{
#ifdef _WIN32
	// ��ʼ���׽��ֿ�
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);
#endif
	// �����׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		cout << "���󣬴����׽���ʧ�ܣ�" << endl;
	}
	cout << "�����׽��ֳɹ���" << endl;

	// �󶨶˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(6100);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		cout << "���󣬰󶨶˿�ʧ�ܣ�" << endl;
	}
	cout << "�󶨶˿ڳɹ���" << endl;

	// �����˿�
	if (SOCKET_ERROR == listen(_sock, 5)) {
		cout << "���󣬼����˿�ʧ�ܣ�" << endl;
	}
	cout << "�����˿ڳɹ���" << endl;

	while (true)
	{
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
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			// ��������
			sockaddr_in clientAddr = {};
			int addrLen = sizeof(sockaddr_in);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
			if (INVALID_SOCKET == _cSock)
			{
				cout << "���󣬽��յ���Ч�Ŀͻ��ˣ�" << endl;
			}
			else
			{
				NewUserJoin userJoin;
				userJoin.sock = _cSock;
				for (auto _cSock : g_clients)
				{
					send(_cSock, (const char*)&userJoin, sizeof(NewUserJoin), 0);
				}
				g_clients.push_back(_cSock);
				cout << "�¿ͻ��˼��룺socket = " << _cSock << "��"
					<< "IP = " << inet_ntoa(clientAddr.sin_addr) << endl;
			}
		}
#ifdef _WIN32
		for (unsigned int i = 0; i < fdRead.fd_count; i++)
		{
			if (-1 == processor(fdRead.fd_array[i]))
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
	}

#ifdef _WIN32
	// �ر��׽���
	for (auto _cSock : g_clients)
	{
		closesocket(_cSock);
	}
	closesocket(_sock);

	// �����׽��ֿ�
	WSACleanup();
#else
	for (auto _cSock : g_clients)
	{
		close(_cSock);
	}
	close(_sock);
#endif
	cout << "���˳���" << endl;

	return 0;
}
