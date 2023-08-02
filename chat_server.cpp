#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <udt.h>
#include <cstdlib>
#include <netdb.h>

using namespace std;
void* send_msg(void*);
void* recv_msg(void*);
char flag = 0;
int main()
{
	//创建套接字
	UDTSOCKET serv = UDT::socket(AF_INET, SOCK_DGRAM, 0);
	
	sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(9000);
	my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(my_addr.sin_zero), '\0', 0);
	//绑定套接字地址
	if(UDT::ERROR == UDT::bind(serv, (sockaddr*)&my_addr, sizeof(my_addr)))
	{
		cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}
	//监听信道
	UDT::listen(serv, 10);
	//获取客户端连接
	int namelen;
	sockaddr_in their_addr;
	UDTSOCKET recver = UDT::accept(serv, (sockaddr*)&their_addr, &namelen);
	cout << "new connection: " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << endl;
	//创建线程变量
	pthread_t send_thread;
	pthread_t recv_thread;
	//启动线程
	pthread_create(&send_thread, NULL, send_msg, new UDTSOCKET(recver));
	pthread_create(&recv_thread, NULL, recv_msg, new UDTSOCKET(recver));
	//等待线程结束
	pthread_join(send_thread, NULL);
	pthread_join(recv_thread, NULL);
	//释放资源
	UDT::close(recver);
	UDT::close(serv);
	
	return 1;
	
}

void* send_msg(void* usocket)
{
	//定义一个套接字变量保存客户端地址端口信息
	UDTSOCKET send_client = *(UDTSOCKET*)usocket;
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
			UDT::sendmsg(send_client, msg, size, -1, false);
			flag = 1;
		}
		else
		{
			//获取信息长度
			size = strlen(data);
			//发送信息
			UDT::sendmsg(send_client, data, size, -1, false);
		}
	}
	UDT::close(send_client);
}

void* recv_msg(void* usocket)
{
	//定义一个套接字变量保存客户端地址端口信息
	UDTSOCKET recv_client = *(UDTSOCKET*)usocket;
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
		size = UDT::recvmsg(recv_client, data, sizeof(data));
		if(strcmp(data, "exit") == 0)
		{
			flag = 1;
		}
		//打印数据信息
		cout << "data: " << data << " size:" << size << endl;
	}
	UDT::close(recv_client);
}
	
	
	
	
	
	
	
	
	
