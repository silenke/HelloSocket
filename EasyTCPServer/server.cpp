#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>
#include <vector>

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
		len = sizeof(NewUserJoin);
	}
	int sock;
};

vector<SOCKET> g_clients;

int processor(SOCKET _cSock)
{
	// 接收数据
	char szRecv[1024]{};
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	if (nLen <= 0) {
		cout << "客户端（" << _cSock << "）已退出，结束任务！" << endl;
		return -1;
	}
	// if (nLen >= sizeof(DataHeader)) 可能少包
	DataHeader* header = (DataHeader*)szRecv;

	// 处理请求
	cout << "收到命令：" << header->cmd << " "
		<< "数据长度：" << header->len << endl;

	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		// 接收数据
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		Login* login = (Login*)szRecv;
		cout << "收到命令：CMD_LOGIN，" << "数据长度：" << header->len << endl
			<< "username=" << login->username << " "
			<< "password=" << login->password << endl;

		// 发送数据
		LoginResult res;
		send(_cSock, (const char*)&res, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		// 接收数据
		recv(_cSock, szRecv + sizeof(DataHeader), header->len - sizeof(DataHeader), 0);
		Logout* logout = (Logout*)szRecv;
		cout << "收到命令：CMD_LOGIN，" << "数据长度：" << header->len << endl
			<< "username=" << logout->username << endl;

		// 发送数据
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
	// 初始化套接字库
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	// 创建套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		cout << "错误，创建套接字失败！" << endl;
	}
	cout << "创建套接字成功！" << endl;

	// 绑定端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(6100);
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		cout << "错误，绑定端口失败！" << endl;
	}
	cout << "绑定端口成功！" << endl;

	// 监听端口
	if (SOCKET_ERROR == listen(_sock, 5)) {
		cout << "错误，监听端口失败！" << endl;
	}
	cout << "监听端口成功！" << endl;

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
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0)
		{
			cout << "select任务结束！" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			// 接受连接
			sockaddr_in clientAddr = {};
			int addrLen = sizeof(sockaddr_in);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
			if (INVALID_SOCKET == _cSock)
			{
				cout << "错误，接收到无效的客户端！" << endl;
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
				cout << "新客户端加入：socket = " << _cSock << "，"
					<< "IP = " << inet_ntoa(clientAddr.sin_addr) << endl;
			}
		}
		for (auto _cSock : g_clients)
		{
			if (FD_ISSET(_cSock, &fdRead) && -1 == processor(_cSock))
			{
				auto it = find(g_clients.begin(), g_clients.end(), _cSock);
				if (it != g_clients.end()) g_clients.erase(it);
			}
		}
	}

	// 关闭套接字
	for (auto _cSock : g_clients)
	{
		closesocket(_cSock);
	}
	closesocket(_sock);

	// 清理套接字库
	WSACleanup();
	cout << "已退出！" << endl;

	getchar();
	return 0;
}
