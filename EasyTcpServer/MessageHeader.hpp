#ifndef _MESSAGE_HEADER_HPP_
#define _MESSAGE_HEADER_HPP_

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_C2S_HEART,
    CMD_S2C_HEART,
    CMD_ERROR
};

struct DataHeader
{
    DataHeader()
    {
        dataLength = sizeof(DataHeader);
        cmd = CMD_ERROR;
    }
    unsigned short dataLength; //数据长度
    unsigned short cmd; //命令
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
        //memset(data, 0, sizeof(data));
    }
    char userName[32];
    char PassWord[32];
    //char data[32];
};

struct LoginResult :public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
        //memset(data, 0, sizeof(data));
    }
    int result;
    //char data[92];
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

struct NewUserJoin :public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(NewUserJoin);
        cmd = CMD_NEW_USER_JOIN;
        sock = 0;
    }
    int sock;
};

struct C2S_Heart :public DataHeader
{
    C2S_Heart()
    {
        dataLength = sizeof(C2S_Heart);
        cmd = CMD_C2S_HEART;
    }
};

struct S2C_Heart :public DataHeader
{
    S2C_Heart()
    {
        dataLength = sizeof(S2C_Heart);
        cmd = CMD_S2C_HEART;
    }
};

#endif // !_MESSAGE_HEADER_HPP_
