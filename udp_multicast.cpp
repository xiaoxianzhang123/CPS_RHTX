#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <cstdlib>
#include <netdb.h>
#include <pthread.h>
#include <limits>

//代码没有严格的客户端与服务器区分，两份代码是一样的
//启动代码后首先填入本地的IP地址与端口号，回车确定
//然后输入需要与组播地址的IP地址与端口号，回车确定
//双方输入exit确定退出连接，退出程序
//退出时需要双方输入exit，但也可强行ctrl+c
using namespace std;
struct socket_data{
    int sock;
    struct sockaddr_in peeraddrin;
};
//声明函数
void *send_msg(void* arg);
void *recv_msg(void* arg);
//退出标志
int exit_flag = 0;

//主程序
int main()
{
    //定义端口和地址变量
    int my_port;
    char myaddr[12];
    int peer_port;
    char peeraddr[12];
    cout << "请输入本地的IP地址与端口号：" << endl;
    cin >> myaddr >> my_port ;
    cout << "请输入组播组的IP地址与端口号："<< endl;
    cin >> peeraddr >> peer_port;
    //清除前面的输入缓存
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //创建套接字
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    /*-----------------*/
    //初始话enable
    int enable = 1;
    // 设置套接字选项，允许发送和接收组播数据
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(IP_MULTICAST_LOOP) failed");
        return 0;
    }
    /*-----------------*/
    //定义本地网络参数
    //创建本地地址结构体
    struct sockaddr_in my_addr;
    //初始化为0
    memset(&my_addr, 0, sizeof(my_addr));
    //设置协议族为IPv4
    my_addr.sin_family = AF_INET;
    //设置端口号（转换为网络字节序）
    my_addr.sin_port = htons((int)my_port);
    //设置IP地址
    inet_pton(AF_INET,myaddr, &my_addr.sin_addr);
    //绑定套接字
    int bind_ret = bind(sock,(struct sockaddr*)&my_addr, sizeof(my_addr));
    if(bind_ret == -1)
    {
        perror("bind errror");
        return 0;
    }
    //定义对端网络参数
    //定义对端网络地址结构体
    struct sockaddr_in peer;
    //初始化为0
    memset(&peer, 0, sizeof(peer));
    //设置为IPv4协议族
    peer.sin_family = AF_INET;
    //设置端口号
    peer.sin_port = htons((int)peer_port);
    //设置对端的IP地址
    inet_pton(AF_INET,peeraddr, &peer.sin_addr);
    //保存对端的结构体数据
    struct socket_data peer_data;
    peer_data.sock = sock;
    peer_data.peeraddrin = peer;

    // 加入组播组
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(peeraddr);  // 组播地址
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);    // 使用默认网络接口
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        perror("setsockopt(IP_ADD_MEMBERSHIP) failed");
        return 0;
    }

    // 设置组播数据目的地
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(peeraddr);  // 组播地址
    multicast_addr.sin_port = htons((int)peer_port);
    
    //创建子线程
    pthread_t send_thread;
    pthread_t recv_thread;
    //启动线程
    pthread_create(&send_thread,  NULL, send_msg, &peer_data);
    pthread_create(&recv_thread, NULL, recv_msg, &peer_data);
    //堵塞等待子线程结束
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 1;
} 

void* send_msg(void* arg)
{
    //定义一个套接字接受保存对端的地址端口信息
    struct socket_data *peer = (struct socket_data*)arg;
    //定义发送缓存
    char data[200];
    string indata;
    int size = 0;

    while(1)
    {
        cout << "请输入需要发送的信息：" <<endl;
        //从标准输入获取一行数据
        std::getline(std::cin, indata);
        //复制输入数据到发送缓存
        strcpy(data, indata.c_str());
        //获取信息长度
        size = strlen(data);
        //发送信息
        ssize_t send_len = sendto(peer->sock, data, sizeof(data), 0,
                                  (struct sockaddr *)&(peer->peeraddrin),
                                  sizeof(peer->peeraddrin)); 
        // ssize_t send_len = sendto(peer->sock, data, sizeof(data), 0,
        //                   (struct sockaddr *)&multicast_addr,
        //                   sizeof(multicast_addr));
        if(send_len == -1)
        {
            perror("sendto error");
            return NULL;
        }
        if(indata == "exit")
        {
            exit_flag = 1;
            break;
        }
    }
    return NULL;
}

void* recv_msg(void* arg)
{
    //定义一个套接字接受保存对端的地址端口信息
    struct socket_data *peer = (struct socket_data *)arg;
    //定义接收缓存
    char data[200];
    int recv_len = 0;
    while(1)
    {
         socklen_t addr_len = sizeof(peer->peeraddrin);   //定义地址长度变量
        //接收对端传来的信息
        ssize_t recv_len = recvfrom(peer->sock, data, sizeof(data), 0, 
                    (struct sockaddr*)&(peer->peeraddrin),
                    &addr_len);
        // ssize_t recv_len = recvfrom(peer->sock, data, sizeof(data), 0,
        //                     (struct sockaddr *)&multicast_addr,
        //                     &addr_len);
        if(recv_len == -1)
        {
            perror("recvfrom error");
            return NULL;
        }
        if(strcmp(data, "exit") == 0)
        {
            cout << "对方已经退出连接，请输入exit退出连接" << endl;
            exit_flag = 1;
            break;
        }
        //打印接收到的数据
        cout << data << endl;
    }
    return NULL;
}