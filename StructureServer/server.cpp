#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <vector>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
//用一个简单的数据结构来在服务端和客户端传递消息
//注意：客户端和服务端之间的字节顺序需要一致
struct DataPackage
{
	int age;
	char name[32];
};

//2018.1.1--徐旭封装了集成的包体格式
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
	//5.接收客户端的数据
	//接收大小为256,而不是128
	int recvLen = recv(real_client_sock, (char*)&data_head, sizeof(DataHeader), 0);
	if (recvLen <= 0)
	{
		printf("客户端已经退出，任务结束\n");
		return -1;
	}
	else {
		//如果接收缓冲区的大小是发送缓冲区的2倍，那么将连续接收两次，造成一定的困扰
		switch (data_head.cmd)
		{
		case CMD_LOGIN:
		{
			Login login = {};
			recv(real_client_sock, (char*)&login + sizeof(DataHeader),
				sizeof(Login) - sizeof(DataHeader), 0);
			printf("收到命令:CMD_LOGIN,数据长度 = %d,username = %s,passwd = %s\n",
				login.dataLength, login.username, login.password);
			//忽略区判断用户名和密码是否正确
			LogResult result;
			send(real_client_sock, (char*)&result, sizeof(LogResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout logout = {};//记录是哪个客户退出了
			recv(real_client_sock, (char*)&logout + sizeof(DataHeader),
				sizeof(Logout) - sizeof(DataHeader), 0);
			printf("收到命令:CMD_LOGOUT,数据长度 = %d,username = %s\n",
				logout.dataLength, logout.username);
			//忽略判断用户名和密码是否正确
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
	//建立windows socket 的编程环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	//------服务端（后端）实现步骤6步或7步
	//1.--创建一个socket
	SOCKET sock;
	//创建一个面向数据流的套接字(socket-->stream)
	sock = ::socket(AF_INET, SOCK_STREAM, 0);
	//2.--绑定一个socket（绑定服务器的IP和端口）
	sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(8080);//主机字节序转到网络的字节序
	sock_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (SOCKET_ERROR == ::bind(sock, (sockaddr*)&sock_addr, sizeof(sock_addr)))
	{
		//绑定时报
		printf("...绑定失败...\n");
	}
	else
	{
		printf("成功绑定\n");
	}
	//3.--启动服务器的监听模式
	if (SOCKET_ERROR == ::listen(sock, 256))
	{
		printf("监听网络端口失败\n");
	}
	else
	{
		printf("监听网络端口成功\n");
	}

	//不停地等待不同的客户端的连接请求(需要把ACCEPT放在循环的内部)
	//如果ACCEPT放在循环的外面，那么一次连接建立以后，下面的循环可以处理不间断的交互
	char recvBuff[256] = {};
	while (true)
	{
		//伯克利套接字(berklet socket)
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExcept;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExcept);

		FD_SET(sock,&fdRead);
		FD_SET(sock,&fdWrite);
		FD_SET(sock,&fdExcept);
		//不要用无符号的SIZE_T，会越界
		for (int n = g_clients.size() - 1;n >= 0;n--)
		{
			FD_SET(g_clients[n],&fdRead);
		}

		//最好的参数指的是等待多长时间而返回NULL-->默认不等待
		//最大描述符nfds（socket是一个整数）
		//NULL------------------------------------------>>>>阻塞模式
		//timeout------------------------------------------>>>>非阻塞模式
		timeval t_val = {0,0};
		//int ret = select(sock + 1,&fdRead,&fdWrite,&fdExcept,NULL);
		int ret = select(sock + 1, &fdRead, &fdWrite, &fdExcept, &t_val);
		if (ret < 0)
		{
			printf("select任务结束了.\n");
			break;
		}

		//判断SOCKET是否可读(查询)
		if (FD_ISSET(sock,&fdRead))
		{ 
			FD_CLR(sock, &fdRead);
			//4.--等待客户端（前端）的连接
			sockaddr_in client_addr;
			int accept_len = sizeof(sockaddr_in);
			SOCKET real_client_sock = INVALID_SOCKET;

			//accept最后一个参数-接收的长度一定不能为0
			real_client_sock = ::accept(sock, (sockaddr*)&client_addr, &accept_len);
			if (real_client_sock == INVALID_SOCKET)
			{
				printf("错误,接受到了无效的客户端SOCKET...\n");
			}
			else
			{
				g_clients.push_back(real_client_sock);
				printf("新客户端加入:IP = %s,PORT = %d \n",
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

		//非阻塞模式下面，即使没有多线程，同样地可以在线程的空闲时间做一些其它的事情
		//例如可以向各个客户端发送一些播报等事情
		{


		}
	}

	//6.关闭套接字SOCKET
	for (int m = g_clients.size() - 1; m >= 0; m--)
	{
		closesocket(g_clients[m]);
	}
	closesocket(sock);

	WSACleanup();
	printf("服务端退出.\n");
	getchar();
	return 0;
}