#include "multicast.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <cstring>

using namespace std;

int mesh_signal_strength[MESH_NODE_NUM][MESH_NODE_NUM/4];
int fiveG_signal_strength[MESH_NODE_NUM][MESH_NODE_NUM/4];
//int fiveG_global_variable[32][8]; // 假设消息大小不超过 1024 字节 存放5g拓扑的全局变量

//int mesh_global_variable[MESH_NODE_NUM][MESH_NODE_NUM/4];   //存放mesh拓扑的全局变量

int send_fiveG_tuopu(const string& mesh_ip, const string& multicast_ip, int fiveG_topu_source[FIVEG_NODE_NUM][FIVEG_NODE_NUM/4]) {
    // 创建组播套接字
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return -1;
    }

    // 设置组播发送的 TTL（Time To Live）值
    int ttl = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ttl, sizeof(ttl)) < 0) {
        cerr << "Failed to set TTL" << endl;
        close(sock);
        return -1;
    }
    
    // 设置组播的源 IP 地址
    struct in_addr ifaddr;
    ifaddr.s_addr = inet_addr(mesh_ip.c_str());
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (void*)&ifaddr, sizeof(ifaddr)) < 0) {
        cerr << "Failed to set multicast source IP" << endl;
        close(sock);
        return -1;
    }

    // 构建组播目标地址结构
    struct sockaddr_in multicast_addr{};
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(multicast_ip.c_str());
    multicast_addr.sin_port = htons(12345); // 设置组播目标端口

    // 发送数据的代码块
    {
        // 将 fiveG_topu_source 数组转换为字符串形式
        string data;
        for (int i = 0; i < FIVEG_NODE_NUM; ++i) {
            for (int j = 0; j < FIVEG_NODE_NUM/4; ++j) {
                data += to_string(fiveG_topu_source[i][j]) + " ";
            }
        }

        // 发送数据到组播地址
        ssize_t sent_bytes = sendto(sock, data.c_str(), data.length(), 0,
                                    (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
        if (sent_bytes < 0) {
            cerr << "Failed to send data" << endl;
            close(sock);
            return -1;
        }

        cout << "Data sent: " << data << endl;

        //sleep(3); // 休眠 3 秒
    }

    // 关闭套接字
    close(sock);

    // 返回发送成功
    return 0;
}


//  ---------------------Mesh电台 从组播组 接收  5G拓扑 -----------------
int recv_fiveG_tuopu(const string& mesh_ip, const string& multicast_ip) {
    // 创建 UDP 套接字
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return -1;
    }

    // 允许多个套接字使用同一端口
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        cerr << "Setting SO_REUSEADDR error" << endl;
        close(sock);
        return -1;
    }

    // 绑定到组播地址和端口
    struct sockaddr_in local_addr{};
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 本地地址
    local_addr.sin_port = htons(12345);
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        close(sock);
        return -1;
    }

    // 加入组播组
    struct ip_mreq group{};
    group.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
    group.imr_interface.s_addr = inet_addr(mesh_ip.c_str());
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
        cerr << "Failed to join multicast group" << endl;
        close(sock);
        return -1;
    }

    // 接收数据
    struct sockaddr_in sender_addr{};
    socklen_t sender_addr_len = sizeof(sender_addr);
    // 假设消息大小不超过 1024 字节
    char msgbuf[1024]; 
    //  // 假设消息大小不超过 1024 字节 存放5g拓扑的全局变量
    {
        int nbytes = recvfrom(sock, msgbuf, sizeof(msgbuf), 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (nbytes < 0) {
            cerr << "Failed to receive data" << endl;
        }
        

        //小小打印一下
        msgbuf[nbytes] = '\0'; // Null-terminate string
        //cout << "Received data: " << msgbuf << endl;

        // TODO: Process the received string and convert it back to the fiveG_tuopu_source array
        // This would involve parsing the string and filling the array with integers.

        //sleep(3); // Match the sender's send interval
    }

    //接收到数据了 开始预处理一下
    std::stringstream ss(msgbuf);  // 将字符数组转换为字符串流

    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::string valueStr;
            ss >> valueStr;  // 从字符串流中读取一个字符串

            // 将字符串转换为整数，并存储到目标数组
            fiveG_signal_strength[i][j] = std::stoi(valueStr);
        }
    }

// 打印目标数组
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << fiveG_signal_strength[i][j] << " ";
        }
        std::cout << std::endl;
    }
    // 离开组播组
    setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group));

    // 关闭套接字
    close(sock);

    // 返回接收成功
    return 0;
}


void send_mesh_tuopu(const string& fiveG_ip1, const string& fiveG_ip2, int mesh_topu_source[MESH_NODE_NUM][MESH_NODE_NUM/4]) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return;
    }

    struct sockaddr_in local_addr{};
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 本地地址
    local_addr.sin_port = htons(12349);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        close(sock);
        return;
    }

    struct sockaddr_in dest_addr{};
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(fiveG_ip2.c_str()); // 目标地址
    dest_addr.sin_port = htons(12349);

    //以下为开始发送的代码块
    {
        if (sendto(sock, mesh_topu_source, MESH_NODE_NUM*(MESH_NODE_NUM/4)*sizeof(int), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            cerr << "Failed to send data" << endl;
            close(sock);
            return;
        }

        cout << "Sent data:" << endl;
        for (int i = 0; i < MESH_NODE_NUM; i++) {
            for (int j = 0; j < MESH_NODE_NUM/4; j++) {
                cout << mesh_topu_source[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;

        //sleep(3); // 等待3秒
    }

    close(sock);
}

// -- 5g接收--
//void recv_mesh_tuopu(const string& fiveG_ip, int mesh_signal_strength[MESH_NODE_NUM][MESH_NODE_NUM/4])
void recv_mesh_tuopu(const string& fiveG_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return;
    }

    struct sockaddr_in local_addr{};
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 本地地址
    local_addr.sin_port = htons(12349);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        close(sock);
        return;
    }

    struct sockaddr_in sender_addr{};
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = inet_addr(fiveG_ip.c_str()); // 发送者地址
    sender_addr.sin_port = htons(12349);

    socklen_t sender_addr_len = sizeof(sender_addr);

    //以下为开始接收的代码块
    {
        if (recvfrom(sock, mesh_signal_strength, MESH_NODE_NUM*(MESH_NODE_NUM/4)*sizeof(int), 0, (struct sockaddr *)&sender_addr, &sender_addr_len) < 0) {
            cerr << "Failed to receive data" << endl;
            close(sock);
            return;
        }

        cout << "Received data from " << inet_ntoa(sender_addr.sin_addr) << ":" << ntohs(sender_addr.sin_port) << endl;
        cout << "Received data:" << endl;
        for (int i = 0; i < MESH_NODE_NUM; i++) {
            for (int j = 0; j < MESH_NODE_NUM/4; j++) {
                cout << mesh_signal_strength[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }

    close(sock);
}


void fiveG_send_fiveG_tuopu(const std::string& fiveG_ip1, const std::string& fiveG_ip2,int fiveG_topu_source[FIVEG_NODE_NUM][FIVEG_NODE_NUM/4]){
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return;
    }

    struct sockaddr_in local_addr{};
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 本地地址
    local_addr.sin_port = htons(12350);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        close(sock);
        return;
    }

    struct sockaddr_in dest_addr{};
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(fiveG_ip2.c_str()); // 目标地址
    dest_addr.sin_port = htons(12350);

    //以下为开始发送的代码块
    {
        if (sendto(sock, fiveG_topu_source, FIVEG_NODE_NUM*(FIVEG_NODE_NUM/4)*sizeof(int), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            cerr << "Failed to send data" << endl;
            close(sock);
            return;
        }

        cout << "Sent data:" << endl;
        for (int i = 0; i < FIVEG_NODE_NUM; i++) {
            for (int j = 0; j < FIVEG_NODE_NUM/4; j++) {    
                cout << fiveG_topu_source[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;

        //sleep(3); // 等待3秒
    }

    close(sock);
}


void fiveG_recv_fiveG_tuopu(const std::string& fiveG_ip){
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return;
    }

    struct sockaddr_in local_addr{};
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 本地地址
    local_addr.sin_port = htons(12350);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        cerr << "Failed to bind socket" << endl;
        close(sock);
        return;
    }

    struct sockaddr_in sender_addr{};
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = inet_addr(fiveG_ip.c_str()); // 发送者地址
    sender_addr.sin_port = htons(12350);

    socklen_t sender_addr_len = sizeof(sender_addr);

    //以下为开始接收的代码块
    {
        int msgbuf[16][4];
        if (recvfrom(sock, msgbuf, FIVEG_NODE_NUM*(FIVEG_NODE_NUM/4)*sizeof(int), 0, (struct sockaddr *)&sender_addr, &sender_addr_len) < 0) {
            cerr << "Failed to receive data" << endl;
            close(sock);
            return;
        }
        cout << "Received data from " << inet_ntoa(sender_addr.sin_addr) << ":" << ntohs(sender_addr.sin_port) << endl;
        cout << "Received data:" << endl;
        for (int i = 0; i < FIVEG_NODE_NUM; i++) {
            for (int j = 0; j < FIVEG_NODE_NUM/4; j++) {
                fiveG_signal_strength[i][j] = msgbuf[i][j];
                //cout << fiveG_signal_strength[i][j] << " ";  
            }
            cout << endl;
        }
        for(int i = 0; i < 32 ; i ++){
            for(int j = 0; j < 8; j ++){
                cout << fiveG_signal_strength[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }

    close(sock);
}


