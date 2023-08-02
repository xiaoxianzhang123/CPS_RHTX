#include <udt.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>
using namespace std;
int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }
    // 初始化UDT库
    UDT::startup();
    // 创建UDT套接字
    UDTSOCKET serv_sock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    // 绑定套接字到指定端口
    sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[1]));
    my_addr.sin_addr.s_addr = INADDR_ANY;
    UDT::bind(serv_sock, (sockaddr*)&my_addr, sizeof(my_addr));
    // 监听套接字
    UDT::listen(serv_sock, 10);
    cout << "Waiting for connection..." << endl;
    // 接受客户端连接请求
    sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    UDTSOCKET client_sock = UDT::accept(serv_sock, (sockaddr*)&client_addr, &addr_len);
    cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << endl;
    // 接收文件名
    char filename[256] = {0};
    UDT::recv(client_sock, filename, sizeof(filename), 0);
    cout << "File name: " << filename << endl;
    // 打开文件
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        cerr << "Failed to open file: " << filename << endl;
        UDT::close(client_sock);
        UDT::close(serv_sock);
        UDT::cleanup();
        return 1;
    }
    // 开始计时器
    auto start_time = chrono::high_resolution_clock::now();
    // 发送文件数据
    int send_count = 0;
    char buffer[1024] = {0};
    while (!feof(fp)) {
        int read_count = fread(buffer, 1, sizeof(buffer), fp);
        int send_size = UDT::send(client_sock, buffer, read_count, 0);
        send_count += send_size;
    }
    // 停止计时器
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    double speed = (double)send_count / duration.count() * 1000 / 1024 / 1024;
    cout << "File sent: " << send_count << " bytes in " << duration.count() << " ms, speed: " << speed << " MB/s" << endl;
    // 关闭文件和套接字
    fclose(fp);
    UDT::close(client_sock);
    UDT::close(serv_sock);
    UDT::cleanup();
    return 0;
}