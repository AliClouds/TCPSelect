#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")

//2018.1.1
//SOCKET���ݱ��� == ��ͷ + ����
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR,
};


struct DataHeader
{
	short dataLength;
	short cmd;
};


//���ݰ�
struct DataPackage
{
	int age;
	char name[32];
};


struct Login:public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char username[32];
	char password[32];
};


struct LogResult:public DataHeader
{
	LogResult()
	{
		dataLength = sizeof(LogResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};


struct Logout:public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd =  CMD_LOGOUT;
	}
	char username[32];
};


struct LogoutResult:public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};



int main()
{
	//��ʼ��SOCKET����
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	//1.����һ��SOCKET
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);//0
	if (sock == SOCKET_ERROR)
	{
		printf("����SOCKETʧ��\n");
	}
	else
	{
		printf("����SOCKET�ɹ�\n");
	}
	//2.connect���ӵ�������
	sockaddr_in server_in = {};
	server_in.sin_family = AF_INET;
	server_in.sin_addr.S_un.S_addr = inet_addr("192.168.1.4");
	server_in.sin_port = htons(8080);
	int ret = connect(sock, (sockaddr*)&server_in, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		printf("���ӷ�����ʧ��.\n");
	}
	else
	{
		printf("���ӷ������ɹ�.\n");
	}
		

	//
	while (true)
	{
		//3.�ͻ���������
		char cmdBuff[128] = {};
		scanf("%s", cmdBuff);//�����û�������(cmd����)
		//4.������������
		if (strcmp(cmdBuff,"exit") == 0)
		{
			printf("�յ��û���EXIT������������.\n");
			break;
		}
		else if (strcmp(cmdBuff,"login")==0)
		{
			Login login;
			strcpy(login.username,"xuxu");
			strcpy(login.password,"123");
			DataHeader data_head = {sizeof(login),CMD_LOGIN};
			//��������������������
			send(sock, (const char*)&login, sizeof(Login), 0);//����
			//���շ�����������
			LogResult log_res = {};
			recv(sock, (char*)&log_res, sizeof(LogResult), 0);

			printf("LogResult:%d \n",log_res.result);
		}
		else if (strcmp(cmdBuff,"logout")==0)
		{
			Logout log_out;
			strcpy(log_out.username, "xuxu");
			//�������������������
			send(sock, (const char*)&log_out, sizeof(Logout), 0);
            //���շ��������صĵǳ����
			LogoutResult log_out_res = {};
			recv(sock,(char*)&log_out_res, sizeof(LogoutResult), 0);

			printf("LogoutResult:%d \n", log_out_res.result);
		}
		else
		{
			printf("�յ���֧�ֵ��������������\n");
		}




	}


	//7.�ر�SOCKET
	closesocket(sock);

	//������
	WSACleanup();

	//���ô���һ������
	getchar();
	return 0;
}