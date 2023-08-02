#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <udt.h>
#include <cstdlib>
#include <netdb.h>


using namespace std;
using namespace UDT;
void* send_msg(void*);
void* recv_msg(void*);
char flag = 0;
int main()
{
	int port;
	char addr[12];
	UDTSOCKET client = UDT::socket(AF_INET, SOCK_DGRAM, 0);
	//客户端地址端口
	sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8080);
	my_addr.sin_addr.s_addr = inet_addr("192.168.10.2");
	memset(&(my_addr.sin_zero), '\0', 0);
	//绑定套接字地址
	if(UDT::ERROR == UDT::bind(client, (sockaddr*)&my_addr, sizeof(my_addr)))
	{
		cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}
	//服务器端地址端口
	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	cout << "请输入服务器IP地址与端口号：" << endl;
	cin >> addr >> port;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, addr, &serv_addr.sin_addr);
	//连接服务器
	memset(&(serv_addr.sin_zero), '\0', 8);
	if(UDT::ERROR == UDT::connect(client, (sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		cout << "connect:" << UDT::getlasterror().getErrorMessage();
		return 0;
	}
	//创建线程变量
	pthread_t send_thread;
	pthread_t recv_thread;
	//启动线程
	pthread_create(&send_thread, NULL, send_msg, new UDTSOCKET(client));
	pthread_create(&recv_thread, NULL, recv_msg, new UDTSOCKET(client));
	//等待线程结束
	pthread_join(send_thread, NULL);
	pthread_join(recv_thread, NULL);
	UDT::close(client);
	return 1;
}

void* send_msg(void* usocket)
{
	//定义一个套接字变量保存客户端地址端口信息
	UDTSOCKET send_server = *(UDTSOCKET*)usocket;
	//释放原端口信息
	delete (UDTSOCKET*)usocket;
	//定义发送缓存
	char data[200];
	int size = 0;
	//循环聊天
	while(1)
	{
		if(flag == 1)
		{
			return 0;
		}
		//输入需要发送的信息
		cout << "请输入需要发送的信息：" << endl;
		//输入信息
		cin >> data;
		//判断是否是推出信息
		if(strcmp(data, "exit") == 0)
		{
			char* msg = "exit";
			//获取信息长度
			size = strlen(msg);
			//发送信息
			UDT::sendmsg(send_server, msg, size, -1, false);
			flag = 1;
		}
		else
		{
			//获取信息长度
			size = strlen(data);
			//发送信息
			UDT::sendmsg(send_server, data, size, -1, false);
		}
	}
	UDT::close(send_server);
}

void* recv_msg(void* usocket)
{
	//定义一个套接字变量保存客户端地址端口信息
	UDTSOCKET recv_server = *(UDTSOCKET*)usocket;
	//释放原端口信息
	delete (UDTSOCKET*)usocket;
	//定义接收缓存
	char data[200];
	int size = 0;
	//循环聊天
	while(1)
	{
		if(flag == 1)
		{
			return 0;
		}
		//接收数据
		size = UDT::recvmsg(recv_server, data, sizeof(data));
		if(strcmp(data, "exit") == 0)
		{
			flag = 1;
		}
		//打印数据信息
		cout << "data: " << data << " size:" << size << endl;
	}
	UDT::close(recv_server);
}
	



