#include <iostream>
#include "EasyTCPServer.hpp"

using namespace std;

int main()
{
	EasyTCPServer server;
	server.InitSocket();
	server.Bind(nullptr, 6100);
	server.Listen(5);

	EasyTCPServer server2;
	server2.InitSocket();
	server2.Bind(nullptr, 6200);
	server2.Listen(5);

	while (server.isRun() || server2.isRun())
	{
		server.OnRun();
		server2.OnRun();
	}

	server.Close();
	server2.Close();
	cout << "ÒÑÍË³ö£¡" << endl;

	Sleep(10000);
	return 0;
}
