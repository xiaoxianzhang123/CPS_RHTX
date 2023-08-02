#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <udt.h>

using namespace std;

void* recvdata(void*);

// 定义一个 UDTUpDown 结构体，用于自动启动和关闭 UDT 库
struct UDTUpDown {
    UDTUpDown() {
        // 使用 UDT::startup() 函数来初始化 UDT 库
        UDT::startup();
    }
    ~UDTUpDown() {
        // 使用 UDT::cleanup() 函数来释放 UDT 库
        UDT::cleanup();
    }
};

int main(int argc, char* argv[]) 
{
   if ((1 != argc) && ((2 != argc) || (0 == atoi(argv[1])))) 
   {
      cout << "usage: appserver [server_port]" << endl;
      return 0;
   }

    // 自动启动和关闭 UDT 模块
    UDTUpDown _udt_;

    addrinfo hints;
    addrinfo* res;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // 指定协议类型为流式传输协议
    //hints.ai_socktype = SOCK_DGRAM; // 指定协议类型为数据报传输协议

    string service("9000");
    if (2 == argc)
        service = argv[1];

    // 获取地址信息
    if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res)) {
        cout << "illegal port number or port is busy.\n" << endl;
        return 0;
    }

    // 创建 UDT 套接字
    UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // UDT 选项
    //UDT::setsockopt(serv, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
    //UDT::setsockopt(serv, 0, UDT_MSS, new int(9000), sizeof(int));
    //UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(serv, 0, UDP_RCVBUF, new int(10000000), sizeof(int));

    // 绑定套接字到本地地址和端口
    if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen)) {
        cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    freeaddrinfo(res);

    cout << "server is ready at port: " << service << endl;

    // 监听连接请求
    if (UDT::ERROR == UDT::listen(serv, 10)) {
        cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);

    UDTSOCKET recver;

    // 接受客户端连接请求并创建新线程处理数据接收
    while (true) 
    {
        if (UDT::INVALID_SOCK == (recver = UDT::accept(serv, (sockaddr*) &clientaddr, &addrlen))) 
        {
            cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
            return 0;
        }

        char clienthost[NI_MAXHOST];
        char clientservice[NI_MAXSERV];
        getnameinfo((sockaddr *) &clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice,
                    sizeof(clientservice), NI_NUMERICHOST | NI_NUMERICSERV);
        cout << "new connection: " << clienthost << ":" << clientservice << endl;

        // 创建新线程来处理数据接收
        pthread_t rcvthread;
        pthread_create(&rcvthread, NULL, recvdata, new UDTSOCKET(recver));
        pthread_detach(rcvthread);
    }

    // 关闭套接字
    UDT::close(serv);

    return 0;
}

void* recvdata(void* usocket) {
    UDTSOCKET recver = *(UDTSOCKET*) usocket;
    delete (UDTSOCKET*) usocket;

    char* data;
    int size = 100000;
    data = new char[size];

    // 接收客户端发送数据的线程函数
    while (true) {
        int rsize = 0;
        int rs;
        while (rsize < size) {
            int rcv_size;
            int var_size = sizeof(int);
            UDT::getsockopt(recver, 0, UDT_RCVDATA, &rcv_size, &var_size);
            if (UDT::ERROR == (rs = UDT::recv(recver, data + rsize, size - rsize, 0))) {
                cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
                break;
            }

            rsize += rs;
        }

        if (rsize < size)
            break;
    }

    delete[] data;

    // 关闭套接字
    UDT::close(recver);

    return NULL;
}