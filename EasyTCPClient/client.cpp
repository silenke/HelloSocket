#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

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
		cout << "�������Ӷ˿�ʧ�ܣ�" << endl;
	}
	cout << "���Ӷ˿ڳɹ���" << endl;

	// ��������
	char buffer[256] = {};
	int len = recv(_sock, buffer, 256, 0);
	if (len > 0) {
		cout << "���յ����ݣ�" << buffer << endl;
	}

	// �ر��׽���
	closesocket(_sock);

	// �����׽��ֿ�
	WSACleanup();

	getchar();
	return 0;
}
