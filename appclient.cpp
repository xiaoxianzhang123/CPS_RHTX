#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <udt.h>
 
using namespace std;
 
// 定义一个辅助函数，用于监控 UDT 的性能
void* monitor(void*);
 
// 定义一个结构体，用于在程序结束时释放 UDT 的资源
struct UDTUpDown {
    UDTUpDown() {
        // 用 UDT::startup() 函数来初始化 UDT 库
        UDT::startup();
    }
    UDTUpDown() {
        // 用 UDT::cleanup() 函数来释放 UDT 库的资源
        UDT::cleanup();
    }
};
 
int main(int argc, char* argv[]) {
    // 检查输入参数是否正确
    if ((3 != argc) || (0 == atoi(argv[2]))) {
        cout << "usage: appclient server_ip server_port" << endl;
        return 0;
    }
 
    // 自动启动和清理 UDT 模块
    UDTUpDown _udt_;
 
    // 定义 addrinfo 结构体并初始化
    struct addrinfo hints, *local, *peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
 
    // 调用 getaddrinfo() 函数解析本地地址
    if (0 != getaddrinfo(NULL, "9000", &hints, &local)) {
        cout << "incorrect network address.\n" << endl;
        return 0;
    }
 
    // 创建 UDT 套接字
    UDTSOCKET client = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
 
    // 设置 UDT 套接字的选项
    //UDT::setsockopt(client, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
    //UDT::setsockopt(client, 0, UDT_MSS, new int(9000), sizeof(int));
    //UDT::setsockopt(client, 0, UDT_SNDBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(client, 0, UDP_SNDBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(client, 0, UDT_MAXBW, new int64_t(12500000), sizeof(int));
 
    // 如果需要使用 Rendezvous Connection，取消下面的注释
    /*
     UDT::setsockopt(client, 0, UDT_RENDEZVOUS, new bool(true), sizeof(bool));
     if (UDT::ERROR == UDT::bind(client, local->ai_addr, local->ai_addrlen))
     {
     cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
     return 0;
     }
     */
 
    freeaddrinfo(local);
 
    // 调用 getaddrinfo() 函数解析服务器地址
    if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer)) {
        cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
        return 0;
    }
 
    // 连接到服务器
    if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen)) {
        cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
 
    freeaddrinfo(peer);
 
    // 准备发送数据
    int size = 100000;
    char* data = new char[size];
 
    // 创建一个线程来监控 UDT 的性能
    pthread_create(new pthread_t, NULL, monitor, &client);
 
    // 发送数据
    for (int i = 0; i < 1000000; i++) {
        int ssize = 0;
        int ss;
        while (ssize < size) {
            if (UDT::ERROR == (ss = UDT::send(client, data + ssize, size - ssize, 0))) {
                cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
                break;
            }
            ssize += ss;
        }
        if (ssize < size)
            break;
    }
 
    // 关闭套接字并释放内存
    UDT::close(client);
    delete[] data;
    return 0;
}
 
// 监控 UDT 的性能
void* monitor(void* s) {
    UDTSOCKET u = *(UDTSOCKET*) s;
    UDT::TRACEINFO perf;
    cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK" << endl;
 
    while (true) {
        // 每隔 1 秒钟输出一次性能信息
        sleep(1);
        if (UDT::ERROR == UDT::perfmon(u, &perf)) {
            cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
            break;
        }
        cout << perf.mbpsSendRate << "\t\t" 
             << perf.msRTT << "\t" 
             << perf.pktCongestionWindow << "\t" 
             << perf.usPktSndPeriod << "\t\t\t" 
             << perf.pktRecvACK << "\t" 
             << perf.pktRecvNAK << endl;
    }
        return NULL;
}