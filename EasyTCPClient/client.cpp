#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

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
		// 发送数据
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (0 == strcmp(cmdBuff, "exit")) {
			cout << "收到退出命令，结束任务！" << endl;
			break;
		}
		send(_sock, cmdBuff, strlen(cmdBuff), 0);

		// 接收数据
		char recvBuff[128] = {};
		int nLen = recv(_sock, recvBuff, 128, 0);
		if (nLen > 0) {
			cout << "接收到数据：" << recvBuff << endl;
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
