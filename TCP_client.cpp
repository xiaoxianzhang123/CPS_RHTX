#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/time.h>
using namespace std;
int main(int argc, char* argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <file_path>" << endl;
        return 1;
    }

    // 创建TCP套接字
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    // 连接到服务器
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    connect(client_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
    cout << "Connected to server: " << argv[1] << ":" << argv[2] << endl;
    // 发送文件名
    char* filename = argv[3];
    send(client_sock, filename, strlen(filename) + 1, 0);

    // 接收文件数据
    int recv_count = 0;
    char buffer[1024] = {0};
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        cerr << "Failed to create file: " << filename << endl;
        close(client_sock);
        
        return 1;
    }
    // 开始计时器
    auto start_time = chrono::high_resolution_clock::now();
    while (true) {
        int recv_size = recv(client_sock, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) break;
        fwrite(buffer, 1, recv_size, fp);
        recv_count += recv_size;
        cout<<"recv_count "<<recv_count<<endl;
    }
    // 停止计时器
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    double speed = (double)recv_count / duration.count() * 1000 / 1024 / 1024;
    cout << "File received: " << recv_count << " bytes in " << duration.count() << " ms, speed: " << speed << " MB/s" << endl;
    // 关闭文件和套接字
    fclose(fp);
    close(client_sock);

    return 0;
}
