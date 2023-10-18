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

using namespace std;
typedef struct {
	int age;
	char name[32];
}DataPackage;


enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
struct DataHeader {
	short dataLength; // ���ݳ���
	CMD cmd; // ����
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
	}
	int code;
	char msg[128];
};

typedef struct LogOut :public DataHeader {
	LogOut() {
		dataLength = sizeof(LogOut);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
}LogOut;

typedef struct LogOutMsg :public DataHeader {
	LogOutMsg() {
		dataLength = sizeof(LogOutMsg);
	}
	char msg[128];
}LogOutMsg;

struct CmdError :public DataHeader {
	CmdError() {
		dataLength = sizeof(CmdError);
	}
	char msg[128];
};

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
		printf("�󶨵�ַ�Ͷ˿�ʧ��\n");
	} else {
		printf("�󶨵�ַ�Ͷ˿ڳɹ�\n");
	}
	ret = listen(_sock, 5);
	assert(ret != -1);

	sockaddr_in clientAddr;
	socklen_t clientAddrLength = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;
	char msgBuf[] = "��ã�����С�գ��ܸ���Ϊ������!";
	int msgLength = strlen(msgBuf) + 1;
	_cSock = accept(_sock, (struct sockaddr *)&clientAddr, &clientAddrLength);
	char remote[INET_ADDRSTRLEN];
	if (_cSock == INVALID_SOCKET) {
		printf("���ܵ���Ч�ͻ���socket\n");
	} else {
		printf("�¿ͻ��˼��룺IP=%s\n", inet_ntop(AF_INET, &clientAddr.sin_addr, remote, INET_ADDRSTRLEN));

		//send(_cSock, msgBuf, msgLength, 0);
	}
	while (true) {
		DataHeader header;
		int reqLen = recv(_cSock, (char *)&header, sizeof(DataHeader), 0);

		if (reqLen == SOCKET_ERROR) {
			printf("�ͻ������˳� \n");
			break;
		}

		switch (header.cmd) {
			case CMD_LOGIN:
			{
				Login loginObj;
				recv(_cSock, (char *)&loginObj + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
				cout << "���յ�����-->CMD_LOGIN" << "���ݰ�����: " << loginObj.cmd << loginObj.dataLength << "��¼�û���: " << loginObj.userName << "��¼����: " << loginObj.passWord << endl;

				// �����˻�������֤
				LoginRes ret;
				ret.code = 0;
				strcpy_s(ret.msg, "��¼�ɹ�");
				send(_cSock, (char *)&ret, sizeof(LoginRes), 0);
			}break;

			case CMD_LOGOUT:
			{
				LogOut logout;
				recv(_cSock, (char *)&logout + sizeof(DataHeader), sizeof(LogOut) - sizeof(DataHeader), 0);
				printf("�û�: %s�˳�\n", logout.userName);

				LogOutMsg ret;
				strcpy_s(ret.msg, "�˳���¼�ɹ�");
				send(_cSock, (char *)&ret, sizeof(ret), 0);
			}break;
			default:
			{
				CmdError error;
				strcpy_s(error.msg, "�������");
				send(_cSock, (char *)&header, sizeof(header), 0);
			}break;
		}

	}


	closesocket(_sock);
	WSACleanup();
	getchar();
	return 0;
}