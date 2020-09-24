#ifndef _EasyTCPClient_hpp_
#define _EasyTCPClient_hpp_


#include "Cell.hpp"
#include "CellClient.hpp"


class EasyTCPClient
{
public:
	EasyTCPClient()
	{
		isConnected = false;
	}

	virtual ~EasyTCPClient()
	{
		Close();
	}

	// ��ʼ���׽���
	void InitSocket()
	{
		CELLNetWork::Init();

		// �����׽���
		if (_pClient)
		{
			CELLlog::Info("<socket=%lld>�رվ�����\n", _pClient->sockfd());
			Close();
		}
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock)
		{
			CELLlog::Info("���󣬴����׽���ʧ�ܣ�\n");
		}
		else
		{
			_pClient = new CellClient(sock);
			//CELLlog::Info("<socket=%lld>�����׽��ֳɹ���\n", _pClient->sockfd());
		}
	}

	// ���ӷ�����
	int Connect(const char* ip, unsigned short port)
	{
		if (!_pClient)
		{
			InitSocket();
		}

		// ��������
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		//CELLlog::Info("<socket=%lld>���ӷ�����<%d:%d>...\n",
		//	_pClient->sockfd(), ip, port);
		int ret = connect(_pClient->sockfd(), (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			CELLlog::Info("<socket=%lld>�������ӷ�����<%d:%d>ʧ�ܣ�\n",
				_pClient->sockfd(), ip, port);
		}
		else
		{
			isConnected = true;
			//CELLlog::Info("<socket=%lld>���ӷ�����<%d:%d>�ɹ���\n",
			//	_pClient->sockfd(), ip, port);
		}
		//std::cout << "<socket=" << _sock << ">connect" << std::endl;
		//std::cout << "<socket=" << _sock << ">���ӷ�����<"
		//	<< ip << ":" << port << ">�ɹ���" << std::endl;

		return ret;
	}

	// �ر��׽���
	void Close()
	{
		if (_pClient)
		{
			delete _pClient;
			_pClient = nullptr;
		}
		isConnected = false;
	}

	// ��ѯ������Ϣ
	//int _nCount = 0;
	bool OnRun()
	{
		if (isRun())
		{
			SOCKET sock = _pClient->sockfd();

			fd_set fdRead;
			fd_set fdWrite;
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_SET(sock, &fdRead);

			timeval t{ 0, 1 };
			int ret = 0;
			if (_pClient->needWrite())
			{
				FD_SET(sock, &fdWrite);
				ret = select(sock + 1, &fdRead, &fdWrite, nullptr, &t);
			}
			else
			{
				ret = select(sock + 1, &fdRead, nullptr, nullptr, &t);
			}
				
			if (ret < 0)
			{
				CELLlog::Info("<socket=%lld>OnRun select exit��\n", sock);
				return false;
			}
			if (FD_ISSET(sock, &fdRead))
			{
				if (-1 == RecvData())
				{
					CELLlog::Info("<socket=%lld>OnRun select RecvData exit��\n", sock);
					Close();
					return false;
				}
			}
			if (FD_ISSET(sock, &fdWrite))
			{
				if (-1 == _pClient->SendDataReal())
				{
					CELLlog::Info("<socket=%lld>OnRun select SendDataReal exit��\n", sock);
					Close();
					return false;
				}
			}

			return true;
		}
		return false;
	}

	bool isRun()
	{
		return _pClient && isConnected;
	}

#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE
	// ������
	char _szRecv[RECV_BUFF_SIZE]{};
	char _szMsgBuf[RECV_BUFF_SIZE * 10]{};
	int _lastPos = 0;

	// ��������
	int RecvData()
	{
		// ��������
		int nLen = _pClient->recvData();
		if (nLen > 0)
		{
			while (_pClient->hasMsg())
			{
				OnNetMsg(_pClient->front_msg());
				_pClient->pop_front_msg();
			}
		}
		return nLen;
	}

	// ��Ӧ������Ϣ
	virtual void OnNetMsg(netmsg_DataHeader* header) = 0;

	// ������Ϣ
	int SendData(netmsg_DataHeader* header)
	{
		return _pClient->SendData(header);
	}

protected:
	CellClient* _pClient = nullptr;
	bool isConnected = false;
};


#endif // _EasyTCPClient_hpp_
