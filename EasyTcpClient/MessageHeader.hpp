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
    DataHeader()
    {
        dataLength = sizeof(DataHeader);
        cmd = CMD_ERROR;
    }
    short dataLength; //���ݳ���
    short cmd; //����
};

//DataPackage
struct Login :public DataHeader
{
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
        memset(userName, 0, sizeof(userName));
        memset(PassWord, 0, sizeof(PassWord));
        memset(data, 0, sizeof(data));
    }
    char userName[32];
    char PassWord[32];
    char data[32];
};

struct LoginResult :public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
        memset(data, 0, sizeof(data));
    }
    int result;
    char data[92];
};

struct Logout :public DataHeader
{
    Logout()
    {
        dataLength = sizeof(Logout);
        cmd = CMD_LOGOUT;
        memset(userName, 0, sizeof(userName));
    }
    char userName[32];
};

struct LogoutResult :public DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 0;
    }
    int result;
};

struct netmsg_NewUserJoin :public DataHeader
{
    netmsg_NewUserJoin()
    {
        dataLength = sizeof(netmsg_NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        sock = 0;
    }
    int sock;
};

#endif //_MessageHeader_hpp_
