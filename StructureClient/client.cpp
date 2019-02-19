#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")

//2018.1.1
//SOCKET数据报文 == 包头 + 包体
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


//数据包
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
	//初始化SOCKET环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);

	//1.建立一个SOCKET
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);//0
	if (sock == SOCKET_ERROR)
	{
		printf("创建SOCKET失败\n");
	}
	else
	{
		printf("创建SOCKET成功\n");
	}
	//2.connect连接到服务器
	sockaddr_in server_in = {};
	server_in.sin_family = AF_INET;
	server_in.sin_addr.S_un.S_addr = inet_addr("192.168.1.4");
	server_in.sin_port = htons(8080);
	int ret = connect(sock, (sockaddr*)&server_in, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		printf("连接服务器失败.\n");
	}
	else
	{
		printf("连接服务器成功.\n");
	}
		

	//
	while (true)
	{
		//3.客户输入命令
		char cmdBuff[128] = {};
		scanf("%s", cmdBuff);//接收用户的输入(cmd窗口)
		//4.处理请求命令
		if (strcmp(cmdBuff,"exit") == 0)
		{
			printf("收到用户的EXIT命令，任务结束了.\n");
			break;
		}
		else if (strcmp(cmdBuff,"login")==0)
		{
			Login login;
			strcpy(login.username,"xuxu");
			strcpy(login.password,"123");
			DataHeader data_head = {sizeof(login),CMD_LOGIN};
			//向服务器发送请求的命令
			send(sock, (const char*)&login, sizeof(Login), 0);//包体
			//接收服务器的数据
			LogResult log_res = {};
			recv(sock, (char*)&log_res, sizeof(LogResult), 0);

			printf("LogResult:%d \n",log_res.result);
		}
		else if (strcmp(cmdBuff,"logout")==0)
		{
			Logout log_out;
			strcpy(log_out.username, "xuxu");
			//向服务器发送请求命令
			send(sock, (const char*)&log_out, sizeof(Logout), 0);
            //接收服务器返回的登出结果
			LogoutResult log_out_res = {};
			recv(sock,(char*)&log_out_res, sizeof(LogoutResult), 0);

			printf("LogoutResult:%d \n", log_out_res.result);
		}
		else
		{
			printf("收到不支持的命令，请重新输入\n");
		}




	}


	//7.关闭SOCKET
	closesocket(sock);

	//清理环境
	WSACleanup();

	//不让窗口一闪而过
	getchar();
	return 0;
}