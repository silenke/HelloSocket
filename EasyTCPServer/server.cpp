#include <iostream>
#include "EasyTCPServer.hpp"

using namespace std;

int main()
{
	EasyTCPServer server;
	server.InitSocket();
	server.Bind(nullptr, 6200);
	server.Listen(5);

	while (server.isRun())
	{
		server.OnRun();
	}

	server.Close();
	cout << "ÒÑÍË³ö£¡" << endl;

	Sleep(10000);
	return 0;
}
