#include <iostream>
#include <thread>
#include "EasyTCPClient.hpp"
#include "CELLTimestamp.hpp"


class MyClient : public EasyTCPClient
{
public:
	// ��Ӧ������Ϣ
	void OnNetMsg(netmsg_DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			// ��������
			netmsg_LoginResult* login = (netmsg_LoginResult*)header;
			//std::cout << "<socket=" << _sock << ">�յ����CMD_LOGIN_RESULT��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl;
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			// ��������
			netmsg_LogoutResult* logout = (netmsg_LogoutResult*)header;
			//std::cout << "<socket=" << _sock << ">�յ����CMD_LOGOUT_RESULT��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl;
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			// ��������
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
			//std::cout << "<socket=" << _sock << ">�յ����CMD_NEW_USER_JOIN��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl;
		}
		break;
		case CMD_ERROR:
		{
			//std::cout << "<socket=" << _sock << ">�յ����CMD_ERROR��"
			//	<< "���ݳ��ȣ�" << header->len << std::endl;
		}
		break;
		default:
		{
			CELLlog::Info("<socket=%lld>�յ�δ֪������ݳ��ȣ�%d\n",
				_pClient->sockfd(), header->len);
		}
		break;
		}
	}

private:
};


bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuff[128]{};
		std::cin >> cmdBuff;
		if (!strcmp(cmdBuff, "exit")) {
			g_bRun = false;
			CELLlog::Info("�յ��˳������������\n");
			return;
		}
		//else if (!strcmp(cmdBuff, "login")) {
		//	// ��������
		//	netmsg_Login login;
		//	strcpy_s(login.username, "Peppa Pig");
		//	strcpy_s(login.password, "15213");
		//	client->SendData(&login);
		//}
		//else if (!strcmp(cmdBuff, "logout")) {
		//	// ��������
		//	netmsg_Logout logout;
		//	strcpy_s(logout.username, "Peppa Pig");
		//	client->SendData(&logout);
		//}
		else
		{
			CELLlog::Info("��֧�ֵ�������������룡\n");
		}
	}
}


const int cCount = 100;
const int tCount = 4;
EasyTCPClient* clients[cCount];
std::atomic_int sendCount = 0;
std::atomic_int readCount = 0;


void recvThread(int begin, int end)
{
	CELLTimestamp t;
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			//if (t.getElapsedSecond() > 3.0 && i == begin)
			//	continue;
			clients[i]->OnRun();
		}
	}
}


void sendThread(int id)
{
	int c = cCount / tCount;
	int begin = id * c;
	int end = id * c + c;
	for (int i = begin; i < end; i++)
	{
		if (!g_bRun)
		{
			for (int j = begin; j < i; j++)
			{
				clients[j]->Close();
				delete clients[j];
			}
			return;
		}
		clients[i] = new MyClient();
		clients[i]->Connect("127.0.0.1", 6100);
		//std::cout << "thread<" << id << ">��Connect=" << i << std::endl;
	}

	CELLlog::Info("thread<%d>��Connect<begin=%d��end=%d>\n", id, begin, end);

	readCount++;
	while (readCount < tCount)	// �ȴ������߳�
	{
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	std::thread t1(recvThread, begin, end);
	t1.detach();

	netmsg_Login login;
	strcpy_s(login.username, "Peppa Pig");
	strcpy_s(login.password, "15213");

	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != clients[i]->SendData(&login))
			{
				sendCount++;
			}
		}
		std::chrono::milliseconds t(99);
		std::this_thread::sleep_for(t);
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->Close();
		delete clients[i];
	}
}


int main()
{
	CELLlog::Instance().setLogPath("clientLog.txt", "w");

	// ����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	// ���������߳�
	for (int i = 0; i < tCount; i++)
	{
		std::thread t1(sendThread, i);
		t1.detach();
	}

	CELLTimestamp tTime;
	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			CELLlog::Info("thread<%d>��time<%d>��clients<%d>��send<%d>\n",
				tCount, t, cCount, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	CELLlog::Info("���˳���\n");

	return 0;
}
