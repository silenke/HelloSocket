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
		cout << "�������ӷ�����ʧ�ܣ�" << endl;
	}
	cout << "���ӷ������ɹ���" << endl;

	while (true)
	{
		// ��������
		char cmdBuff[128]{};
		cin >> cmdBuff;
		if (0 == strcmp(cmdBuff, "exit")) {
			cout << "�յ��˳������������" << endl;
			break;
		}
		send(_sock, cmdBuff, strlen(cmdBuff), 0);

		// ��������
		char recvBuff[128] = {};
		int nLen = recv(_sock, recvBuff, 128, 0);
		if (nLen > 0) {
			cout << "���յ����ݣ�" << recvBuff << endl;
		}
	}

	// �ر��׽���
	closesocket(_sock);

	// �����׽��ֿ�
	WSACleanup();
	cout << "���˳���" << endl;

	getchar();
	return 0;
}
