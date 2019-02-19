#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <vector>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
//��һ���򵥵����ݽṹ���ڷ���˺Ϳͻ��˴�����Ϣ
//ע�⣺�ͻ��˺ͷ����֮����ֽ�˳����Ҫһ��
struct DataPackage
{
	int age;
	char name[32];
};

//2018.1.1--�����װ�˼��ɵİ����ʽ
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



struct Login :public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char username[32];
	char password[32];
};


struct LogResult :public DataHeader
{
	LogResult()
	{
		dataLength = sizeof(LogResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};


struct Logout :public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char username[32];
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


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

vector<SOCKET> g_clients;


int processor(SOCKET real_client_sock)
{
	DataHeader data_head;
	//5.���տͻ��˵�����
	//���մ�СΪ256,������128
	int recvLen = recv(real_client_sock, (char*)&data_head, sizeof(DataHeader), 0);
	if (recvLen <= 0)
	{
		printf("�ͻ����Ѿ��˳����������\n");
		return -1;
	}
	else {
		//������ջ������Ĵ�С�Ƿ��ͻ�������2������ô�������������Σ����һ��������
		switch (data_head.cmd)
		{
		case CMD_LOGIN:
		{
			Login login = {};
			recv(real_client_sock, (char*)&login + sizeof(DataHeader),
				sizeof(Login) - sizeof(DataHeader), 0);
			printf("�յ�����:CMD_LOGIN,���ݳ��� = %d,username = %s,passwd = %s\n",
				login.dataLength, login.username, login.password);
			//�������ж��û����������Ƿ���ȷ
			LogResult result;
			send(real_client_sock, (char*)&result, sizeof(LogResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout logout = {};//��¼���ĸ��ͻ��˳���
			recv(real_client_sock, (char*)&logout + sizeof(DataHeader),
				sizeof(Logout) - sizeof(DataHeader), 0);
			printf("�յ�����:CMD_LOGOUT,���ݳ��� = %d,username = %s\n",
				logout.dataLength, logout.username);
			//�����ж��û����������Ƿ���ȷ
			LogoutResult logout_result;
			send(real_client_sock, (char*)&logout_result, sizeof(LogoutResult), 0);
		}
		break;
		default:
		{
			data_head.cmd = CMD_ERROR;
			data_head.dataLength = 0;
			send(real_client_sock, (char*)&data_head, sizeof(DataHeader), 0);
		}
		break;
		}
	}
	return 0;
}




int main()
{
	//����windows socket �ı�̻���
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	//------����ˣ���ˣ�ʵ�ֲ���6����7��
	//1.--����һ��socket
	SOCKET sock;
	//����һ���������������׽���(socket-->stream)
	sock = ::socket(AF_INET, SOCK_STREAM, 0);
	//2.--��һ��socket���󶨷�������IP�Ͷ˿ڣ�
	sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(8080);//�����ֽ���ת��������ֽ���
	sock_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (SOCKET_ERROR == ::bind(sock, (sockaddr*)&sock_addr, sizeof(sock_addr)))
	{
		//��ʱ��
		printf("...��ʧ��...\n");
	}
	else
	{
		printf("�ɹ���\n");
	}
	//3.--�����������ļ���ģʽ
	if (SOCKET_ERROR == ::listen(sock, 256))
	{
		printf("��������˿�ʧ��\n");
	}
	else
	{
		printf("��������˿ڳɹ�\n");
	}

	//��ͣ�صȴ���ͬ�Ŀͻ��˵���������(��Ҫ��ACCEPT����ѭ�����ڲ�)
	//���ACCEPT����ѭ�������棬��ôһ�����ӽ����Ժ������ѭ�����Դ�����ϵĽ���
	char recvBuff[256] = {};
	while (true)
	{
		//�������׽���(berklet socket)
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExcept;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExcept);

		FD_SET(sock,&fdRead);
		FD_SET(sock,&fdWrite);
		FD_SET(sock,&fdExcept);
		//��Ҫ���޷��ŵ�SIZE_T����Խ��
		for (int n = g_clients.size() - 1;n >= 0;n--)
		{
			FD_SET(g_clients[n],&fdRead);
		}

		//��õĲ���ָ���ǵȴ��೤ʱ�������NULL-->Ĭ�ϲ��ȴ�
		//���������nfds��socket��һ��������
		//NULL------------------------------------------>>>>����ģʽ
		//timeout------------------------------------------>>>>������ģʽ
		timeval t_val = {0,0};
		//int ret = select(sock + 1,&fdRead,&fdWrite,&fdExcept,NULL);
		int ret = select(sock + 1, &fdRead, &fdWrite, &fdExcept, &t_val);
		if (ret < 0)
		{
			printf("select���������.\n");
			break;
		}

		//�ж�SOCKET�Ƿ�ɶ�(��ѯ)
		if (FD_ISSET(sock,&fdRead))
		{ 
			FD_CLR(sock, &fdRead);
			//4.--�ȴ��ͻ��ˣ�ǰ�ˣ�������
			sockaddr_in client_addr;
			int accept_len = sizeof(sockaddr_in);
			SOCKET real_client_sock = INVALID_SOCKET;

			//accept���һ������-���յĳ���һ������Ϊ0
			real_client_sock = ::accept(sock, (sockaddr*)&client_addr, &accept_len);
			if (real_client_sock == INVALID_SOCKET)
			{
				printf("����,���ܵ�����Ч�Ŀͻ���SOCKET...\n");
			}
			else
			{
				g_clients.push_back(real_client_sock);
				printf("�¿ͻ��˼���:IP = %s,PORT = %d \n",
					inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
			}
		}

		for (int n = 0;n < fdRead.fd_count;n++)
		{
			if (-1 == processor(fdRead.fd_array[n]))
			{
				auto iter = find(g_clients.begin(),g_clients.end(),fdRead.fd_array[n]);
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}

		//������ģʽ���棬��ʹû�ж��̣߳�ͬ���ؿ������̵߳Ŀ���ʱ����һЩ����������
		//�������������ͻ��˷���һЩ����������
		{


		}
	}

	//6.�ر��׽���SOCKET
	for (int m = g_clients.size() - 1; m >= 0; m--)
	{
		closesocket(g_clients[m]);
	}
	closesocket(sock);

	WSACleanup();
	printf("������˳�.\n");
	getchar();
	return 0;
}