#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <string>
#define RECV_SIZE 128
#include <vector>
using namespace std;

enum CMD {
	CMD_LOGIN = 1,
	CMD_LOGIN_RESULT = 2,
	CMD_LOGOUT = 3,
	CMD_LOGOUT_RESULT = 4,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
struct DataHeader {
	short dataLength; // 数据长度
	short cmd = NULL; // 命令
};

typedef struct Login : public DataHeader {
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
}Login;

struct LoginRes : public DataHeader {
	LoginRes() {
		dataLength = sizeof(LoginRes);
		cmd = CMD_LOGIN_RESULT;
	}
	int code;
	char msg[128];
};

struct LogOut :public DataHeader {
	LogOut() {
		dataLength = sizeof(LogOut);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

typedef struct LogOutMsg :public DataHeader {
	LogOutMsg() {
		dataLength = sizeof(LogOutMsg);
		cmd = CMD_LOGOUT_RESULT;
	}
	int code;
	char msg[128];
}LogOutMsg;

struct CmdError :public DataHeader {
	CmdError() {
		dataLength = sizeof(CmdError);
	}
	char msg[128];
};

struct NewUserJoin :public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sockId = 0;
	}
	int sockId;
};


vector<SOCKET> gClients;

int processor(SOCKET _cSock) {
	// 作为缓冲区
	char szRecv[1024] = {};
	int reqLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader *header = (DataHeader *)szRecv;

	if (reqLen <= SOCKET_ERROR) {
		printf("客户端已退出 \n");
		return -1;
	}
	// 判断粘包 少包
	/*if (reqLen >= sizeof(DataHeader)) {
		return 0;
	}*/

	switch (header->cmd) {
		case CMD_LOGIN:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Login *loginObj = (Login *)szRecv;
			cout << "接收到命令-->CMD_LOGIN" << "数据包长度: " << loginObj->cmd << loginObj->dataLength << "登录用户名: " << loginObj->userName << "登录密码: " << loginObj->passWord << endl;

			// 忽略账户密码验证
			LoginRes ret;
			ret.code = 0;
			strcpy_s(ret.msg, "登录成功");
			send(_cSock, (char *)&ret, sizeof(LoginRes), 0);
		}break;

		case CMD_LOGOUT:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LogOut *logout = (LogOut *)szRecv;
			printf("用户: %s退出\n", logout->userName);

			LogOutMsg ret;
			ret.code = 0;
			strcpy_s(ret.msg, "退出登录成功");
			send(_cSock, (char *)&ret, sizeof(ret), 0);
		}break;

		default:
		{
			CmdError error;
			strcpy_s(error.msg, "命令错误");
			send(_cSock, (char *)&error, sizeof(CmdError), 0);
		}break;
	}
}

int main() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	SOCKET _sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(_sock >= 0);
	sockaddr_in	_sin;
	memset(&_sin, 0, sizeof(_sin));
	_sin.sin_family = AF_INET;
	const char* serverIp = "127.0.0.1";
	int ret = inet_pton(AF_INET, serverIp, &_sin.sin_addr);
	assert(ret == 1);
	_sin.sin_port = htons(8999);
	ret = bind(_sock, (sockaddr *)&_sin, sizeof(_sin));
	if (ret == SOCKET_ERROR) {
		printf("绑定地址和端口失败\n");
	} else {
		printf("绑定地址和端口成功\n");
	}
	ret = listen(_sock, 5);
	assert(ret != -1);



	while (true) {
		// 伯克利 socket
		fd_set fdRead = {};
		fd_set fdWrite = {};
		fd_set fdExp = {};

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);


		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (int n = (int)gClients.size() - 1; n >= 0; n--) {
			FD_SET(gClients[n], &fdRead);
		}

		timeval t = { 1,0 };

		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);

		if (ret < 0) {
			printf("select 任务结束 \n");
			break;
		}

		// 判断socket 是否可读
		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

			sockaddr_in clientAddr;
			socklen_t clientAddrLength = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (struct sockaddr *)&clientAddr, &clientAddrLength);
			char remote[INET_ADDRSTRLEN];
			if (_cSock == INVALID_SOCKET) {
				printf("接受到无效客户端socket... \n");
			}
			for (int n = (int)gClients.size() - 1; n >= 0; n--) {
				NewUserJoin newUser;
				send(gClients[n], (char *)&newUser, sizeof(NewUserJoin), 0);
			}
			gClients.push_back(_cSock);
			printf("新客户端加入：IP = %s \n", inet_ntop(AF_INET, &clientAddr.sin_addr, remote, INET_ADDRSTRLEN));
		}

		for (size_t n = 0; n < fdRead.fd_count; n++) {
			if (-1 == processor(fdRead.fd_array[n])) {
				auto iter = find(gClients.begin(), gClients.end(), fdRead.fd_array[n]);

				if (iter != gClients.end()) {
					gClients.erase(iter);
				}
				break;
			}
		}
		printf("空闲时间处理其它业务 \n");

	}

	for (int n = (int)gClients.size() - 1; n >= 0; n--) {
		closesocket(gClients[n]);
	}
	closesocket(_sock);
	WSACleanup();
	getchar();
	return 0;
}