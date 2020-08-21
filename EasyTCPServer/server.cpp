#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct DataPackage
{
	int age;
	char name[32];
};

int main()
{
	// ��ʼ���׽��ֿ�
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);

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
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		cout << "���󣬰󶨶˿�ʧ�ܣ�" << endl;
	}
	cout << "�󶨶˿ڳɹ���" << endl;

	// �����˿�
	if (SOCKET_ERROR == listen(_sock, 5)) {
		cout << "���󣬼����˿�ʧ�ܣ�" << endl;
	}
	cout << "�����˿ڳɹ���" << endl;

	sockaddr_in clientAddr = {};
	int addrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;

	// ��������
	_cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
	if (INVALID_SOCKET == _cSock) {
		cout << "���󣬽��յ���Ч�Ŀͻ��ˣ�" << endl;
	}
	cout << "�¿ͻ��˼��룬IP��" << inet_ntoa(clientAddr.sin_addr) << endl;

	while (true)
	{
		// ��������
		char _recvBuff[128]{};
		int nLen = recv(_cSock, _recvBuff, 128, 0);
		if (nLen <= 0) {
			cout << "�ͻ������˳�����������" << endl;
			break;
		}

		// ��������
		if (!strcmp(_recvBuff, "getInfo")) {
			// ��������
			DataPackage dp{24, "Peppa"};
			send(_cSock, (const char*)&dp, sizeof(DataPackage), 0);
		}
		else {
			send(_cSock, "???", strlen("???") + 1, 0);
		}
	}

	// �����׽��ֿ�
	WSACleanup();
	cout << "���˳���" << endl;

	getchar();
	return 0;
}
