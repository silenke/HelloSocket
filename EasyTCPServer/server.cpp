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

	sockaddr_in clientAddr = {};
	int addrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;

	char buffer[] = "Hello!";
	while (true)
	{
		// 接受连接
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == _cSock) {
			cout << "错误，接收到无效的客户端！" << endl;
		}
		cout << "新客户端加入，IP：" << inet_ntoa(clientAddr.sin_addr) << endl;
		
		// 发送数据
		send(_cSock, buffer, sizeof(buffer) + 1, 0);
	}

	// 清理套接字库
	WSACleanup();
	return 0;
}
