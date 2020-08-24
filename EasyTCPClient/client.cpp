#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

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
		len = sizeof(CMD_NEW_USER_JOIN);
	}
	int sock;
};

int processor(SOCKET _cSock)
{
	// ��������
	char szRecv[1024]{};
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0) {
		cout << "��������Ͽ����ӣ���������" << endl;
		return -1;
	}
	// if (nLen >= sizeof(DataHeader)) �����ٰ�
	DataHeader* header = (DataHeader*)szRecv;

	// ��������
	cout << "�յ����" << header->cmd << " "
		<< "���ݳ��ȣ�" << header->len << endl;

	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		// ��������
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		LoginResult* login = (LoginResult*)szRecv;
		cout << "�յ����CMD_LOGIN_RESULT��" << "���ݳ��ȣ�" << header->len << endl;
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		// ��������
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		LogoutResult* logout = (LogoutResult*)szRecv;
		cout << "�յ����CMD_LOGOUT_RESULT��" << "���ݳ��ȣ�" << header->len << endl;
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		// ��������
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		NewUserJoin* userJoin = (NewUserJoin*)szRecv;
		cout << "�յ����CMD_NEW_USER_JOIN��" << "���ݳ��ȣ�" << header->len << endl;
	}
	break;
	case CMD_ERROR:
	{
		cout << "�յ����CMD_ERROR��" << "���ݳ��ȣ�" << header->len << endl;
	}
	break;
	default:
	{
		cout << "�յ�δ֪���" << "���ݳ��ȣ�" << header->len << endl;
	}
	break;
	}

	return 0;
}

bool g_bRun = true;

void cmdThread(SOCKET _sock)
{
	while (true)
	{
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			g_bRun = false;
			cout << "�յ��˳������������" << endl;
			return;
		}
		else if (!strcmp(cmdBuff, "login")) {
			// ��������
			Login login;
			strcpy_s(login.username, "Peppa Pig");
			strcpy_s(login.password, "15213");
			send(_sock, (const char*)&login, sizeof(Login), 0);

			// ��������
			LoginResult res{};
			recv(_sock, (char*)&res, sizeof(LoginResult), 0);
			cout << "LoginResult��" << res.result << endl;
		}
		else if (!strcmp(cmdBuff, "logout")) {
			// ��������
			Logout logout;
			strcpy_s(logout.username, "Peppa Pig");
			send(_sock, (const char*)&logout, sizeof(Logout), 0);

			// ��������
			LogoutResult res{};
			recv(_sock, (char*)&res, sizeof(LogoutResult), 0);
			cout << "LogoutResult��" << res.result << endl;
		}
		else {
			cout << "��֧�ֵ�������������룡" << endl;
		}
	}
}

int main()
{
	// ��ʼ���׽��ֿ�
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	// �����׽���
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		cout << "���󣬴����׽���ʧ�ܣ�" << endl;
	}
	cout << "�����׽��ֳɹ���" << endl;

	// ��������
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(6100);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		cout << "�������ӷ�����ʧ�ܣ�" << endl;
	}
	cout << "���ӷ������ɹ���" << endl;

	thread t1(cmdThread, _sock);
	t1.detach();

	while (g_bRun)
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval t{};
		int ret = select(_sock + 1, &fdRead, NULL, NULL, &t);
		if (ret < 0)
		{
			cout << "select���������" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == processor(_sock))
			{
				break;
			}
		}
	}

	// �ر��׽���
	closesocket(_sock);

	// �����׽��ֿ�
	WSACleanup();
	cout << "���˳���" << endl;

	return 0;
}
