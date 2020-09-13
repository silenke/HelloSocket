#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

struct DataHeader
{
	DataHeader() {
		cmd = CMD_ERROR;
		len = sizeof(DataHeader);
	}
	short cmd;
	short len;
};

struct netmsg_Login : public DataHeader
{
	netmsg_Login() {
		cmd = CMD_LOGIN;
		len = sizeof(netmsg_Login);
	}
	char username[32];
	char password[32];
	char data[956];
};

struct netmsg_LoginResult : public DataHeader
{
	netmsg_LoginResult() {
		cmd = CMD_LOGIN_RESULT;
		len = sizeof(netmsg_LoginResult);
		result = 0;
	}
	int result;
	char data[1016];
};

struct netmsg_Logout : public DataHeader
{
	netmsg_Logout() {
		cmd = CMD_LOGOUT;
		len = sizeof(netmsg_Logout);
	}
	char username[32];
};

struct netmsg_LogoutResult : public DataHeader
{
	netmsg_LogoutResult() {
		cmd = CMD_LOGOUT_RESULT;
		len = sizeof(netmsg_LogoutResult);
		result = 0;
	}
	int result;
};

struct netmsg_NewUserJoin : public DataHeader
{
	netmsg_NewUserJoin() {
		cmd = CMD_NEW_USER_JOIN;
		len = sizeof(netmsg_NewUserJoin);
	}
	SOCKET sock;
};

#endif // !_MessageHeader_hpp_