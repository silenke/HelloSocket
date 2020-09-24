#include <iostream>
#include <thread>
#include "EasyTCPClient.hpp"
#include "CELLTimestamp.hpp"


class MyClient : public EasyTCPClient
{
public:
	// 响应网络消息
	void OnNetMsg(netmsg_DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			// 接收数据
			netmsg_LoginResult* login = (netmsg_LoginResult*)header;
			//std::cout << "<socket=" << _sock << ">收到命令：CMD_LOGIN_RESULT，"
			//	<< "数据长度：" << header->len << std::endl;
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			// 接收数据
			netmsg_LogoutResult* logout = (netmsg_LogoutResult*)header;
			//std::cout << "<socket=" << _sock << ">收到命令：CMD_LOGOUT_RESULT，"
			//	<< "数据长度：" << header->len << std::endl;
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			// 接收数据
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
			//std::cout << "<socket=" << _sock << ">收到命令：CMD_NEW_USER_JOIN，"
			//	<< "数据长度：" << header->len << std::endl;
		}
		break;
		case CMD_ERROR:
		{
			//std::cout << "<socket=" << _sock << ">收到命令：CMD_ERROR，"
			//	<< "数据长度：" << header->len << std::endl;
		}
		break;
		default:
		{
			CELLlog::Info("<socket=%lld>收到未知命令，数据长度：%d\n",
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
			CELLlog::Info("收到退出命令，结束任务！\n");
			return;
		}
		//else if (!strcmp(cmdBuff, "login")) {
		//	// 发送数据
		//	netmsg_Login login;
		//	strcpy_s(login.username, "Peppa Pig");
		//	strcpy_s(login.password, "15213");
		//	client->SendData(&login);
		//}
		//else if (!strcmp(cmdBuff, "logout")) {
		//	// 发送数据
		//	netmsg_Logout logout;
		//	strcpy_s(logout.username, "Peppa Pig");
		//	client->SendData(&logout);
		//}
		else
		{
			CELLlog::Info("不支持的命令，请重新输入！\n");
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
		//std::cout << "thread<" << id << ">，Connect=" << i << std::endl;
	}

	CELLlog::Info("thread<%d>，Connect<begin=%d，end=%d>\n", id, begin, end);

	readCount++;
	while (readCount < tCount)	// 等待其他线程
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

	// 启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	// 启动发送线程
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
			CELLlog::Info("thread<%d>，time<%d>，clients<%d>，send<%d>\n",
				tCount, t, cCount, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	CELLlog::Info("已退出！\n");

	return 0;
}
