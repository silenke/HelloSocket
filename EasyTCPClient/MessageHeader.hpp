#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_HEART_C2S,
	CMD_HEART_S2C,
	CMD_ERROR
};

struct netmsg_DataHeader
{
	netmsg_DataHeader() {
		cmd = CMD_ERROR;
		len = sizeof(netmsg_DataHeader);
	}
	short cmd;
	short len;
};

struct netmsg_Login : public netmsg_DataHeader
{
	netmsg_Login() {
		cmd = CMD_LOGIN;
		len = sizeof(netmsg_Login);
	}
	char username[32];
	char password[32];
	char data[956];
};

struct netmsg_LoginResult : public netmsg_DataHeader
{
	netmsg_LoginResult() {
		cmd = CMD_LOGIN_RESULT;
		len = sizeof(netmsg_LoginResult);
		result = 0;
	}
	int result;
	char data[1016];
};

struct netmsg_Logout : public netmsg_DataHeader
{
	netmsg_Logout() {
		cmd = CMD_LOGOUT;
		len = sizeof(netmsg_Logout);
	}
	char username[32];
};

struct netmsg_LogoutResult : public netmsg_DataHeader
{
	netmsg_LogoutResult() {
		cmd = CMD_LOGOUT_RESULT;
		len = sizeof(netmsg_LogoutResult);
		result = 0;
	}
	int result;
};

struct netmsg_NewUserJoin : public netmsg_DataHeader
{
	netmsg_NewUserJoin() {
		cmd = CMD_NEW_USER_JOIN;
		len = sizeof(netmsg_NewUserJoin);
	}
	SOCKET sock;
};

struct netmsg_Heart_C2S : public netmsg_DataHeader
{
	netmsg_Heart_C2S() {
		cmd = CMD_HEART_C2S;
		len = sizeof(netmsg_Heart_C2S);
	}
};

struct netmsg_Heart_S2C : public netmsg_DataHeader
{
	netmsg_Heart_S2C() {
		cmd = CMD_HEART_S2C;
		len = sizeof(netmsg_Heart_S2C);
	}
};

#endif // !_MessageHeader_hpp_