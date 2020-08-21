#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

enum CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR
};

struct DataHeader
{
	short len;
	short cmd;
};

struct Login
{
	char username[32];
	char password[32];
};

struct LoginResult
{
	int result;
};

struct Logout
{
	char username[32];
};

struct LogoutResult
{
	int result;
};

int main()
{
	// 初始化套接字库
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

	// 创建套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		cout << "错误，创建套接字失败！" << endl;
	}
	cout << "创建套接字成功！" << endl;

	// 发起连接
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(6100);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		cout << "错误，连接服务器失败！" << endl;
	}
	cout << "连接服务器成功！" << endl;

	while (true)
	{
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			cout << "收到退出命令，结束任务！" << endl;
			break;
		}
		else if (!strcmp(cmdBuff, "login")) {
			// 发送数据
			DataHeader dataHeader{ CMD_LOGIN, sizeof(Login) };
			Login login{ "Peppa Pig", "15213" };
			send(_sock, (const char*)&dataHeader, sizeof(DataHeader), 0);
			send(_sock, (const char*)&login, sizeof(Login), 0);

			// 接收数据
			DataHeader resHeader{};
			LoginResult res{};
			recv(_sock, (char*)&resHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&res, sizeof(LoginResult), 0);
			cout << "LoginResult："  << res.result << endl;
		}
		else if (!strcmp(cmdBuff, "logout")) {
			// 发送数据
			DataHeader dataHeader{ CMD_LOGOUT, sizeof(Logout) };
			Logout logout{ "Peppa Pig" };
			send(_sock, (const char*)&dataHeader, sizeof(DataHeader), 0);
			send(_sock, (const char*)&logout, sizeof(Logout), 0);

			// 接收数据
			DataHeader resHeader{};
			LogoutResult res{};
			recv(_sock, (char*)&resHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&res, sizeof(LogoutResult), 0);
			cout << "LogoutResult：" << res.result << endl;
		}
		else {
			cout << "不支持的命令，请重新输入！" << endl;
		}
	}

	// 关闭套接字
	closesocket(_sock);

	// 清理套接字库
	WSACleanup();
	cout << "已退出！" << endl;

	getchar();
	return 0;
}
