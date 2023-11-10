#include "multicast.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <cstring>

using namespace std;

int send_fiveG_tuopu(const string& mesh_ip, const string& multicast_ip, int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]) {
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

    // 发送数据
    while (true) {
        // 将 fiveG_topu_source 数组转换为字符串形式
        string data;
        for (int i = 0; i < ROUTING_NUM; ++i) {
            for (int j = 0; j < ROUTING_NUM; ++j) {
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

        sleep(3); // 休眠 3 秒
    }

    // 关闭套接字
    close(sock);

    // 返回发送成功
    return 0;
}



int recv_fiveG_tuopu(const string& mesh_ip, const string& multicast_ip, int fiveG_tuopu_source[ROUTING_NUM][ROUTING_NUM]) {
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
    char msgbuf[1024]; // 假设消息大小不超过 1024 字节

    while (true) {
        int nbytes = recvfrom(sock, msgbuf, sizeof(msgbuf), 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (nbytes < 0) {
            cerr << "Failed to receive data" << endl;
            break;
        }

        msgbuf[nbytes] = '\0'; // Null-terminate string
        cout << "Received data: " << msgbuf << endl;

        // TODO: Process the received string and convert it back to the fiveG_tuopu_source array
        // This would involve parsing the string and filling the array with integers.

        sleep(3); // Match the sender's send interval
    }

    // 离开组播组
    setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group));

    // 关闭套接字
    close(sock);

    // 返回接收成功
    return 0;
}


// int recv_fiveG_tuopu(const string& mesh_ip, const string& multicast_ip, int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]) {
//     // 创建组播套接字
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         cerr << "Failed to create socket" << endl;
//         return -1;
//     }

//     // 设置组播接收的 IP 地址和端口
//     struct sockaddr_in local_addr{};
//     memset(&local_addr, 0, sizeof(local_addr));
//     local_addr.sin_family = AF_INET;
//     local_addr.sin_addr.s_addr = inet_addr(mesh_ip.c_str());
//     local_addr.sin_port = htons(12345); // 设置组播端口

//     // 绑定套接字到本地地址
//     if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
//         cerr << "Failed to bind socket" << endl;
//         close(sock);
//         return -1;
//     }

//     // 加入到组播组
//     struct ip_mreq mreq{};
//     mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
//     mreq.imr_interface.s_addr = inet_addr(mesh_ip.c_str());
//     if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mreq, sizeof(mreq)) < 0) {
//         cerr << "Failed to join multicast group" << endl;
//         close(sock);
//         return -1;
//     }

//     // 接收数据
//     char buffer[1024];
//     while (true) {
//         memset(buffer, 0, sizeof(buffer));
//         ssize_t recv_bytes = recv(sock, buffer, sizeof(buffer), 0);
//         if (recv_bytes < 0) {
//             cerr << "Failed to receive data" << endl;
//             close(sock);
//             return -1;
//         }

//         // 解析接收到的字符串数据，将其转换为 fiveG_topu_source 数组
//         string data(buffer);
//         stringstream ss(data);
//         int value;
//         for (int i = 0; i < ROUTING_NUM; ++i) {
//             for (int j = 0; j < ROUTING_NUM; ++j) {
//                 ss >> value;
//                 if (!ss) {
//                     cerr << "Failed to parse received data" << endl;
//                     close(sock);
//                     return -1;
//                 }
//                 fiveG_topu_source[i][j] = value;
//             }
//         }

//         // 打印接收到的数据
//         cout << "Data received: ";
//         for (int i = 0; i < ROUTING_NUM; ++i) {
//             for (int j = 0; j < ROUTING_NUM; ++j) {
//                 cout << fiveG_topu_source[i][j] << " ";
//             }
//         }
//         cout << endl;
//     }

//     // 关闭套接字
//     close(sock);

//     // 返回接收成功
//     return 0;
// }



//0-----------------------
void send_mesh_tuopu(const string& fiveG_ip1, const string& fiveG_ip2, int mesh_topu_source[ROUTING_NUM][ROUTING_NUM]) {
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

    while (true) {
        if (sendto(sock, mesh_topu_source, ROUTING_NUM*ROUTING_NUM*sizeof(int), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            cerr << "Failed to send data" << endl;
            close(sock);
            return;
        }

        cout << "Sent data:" << endl;
        for (int i = 0; i < ROUTING_NUM; i++) {
            for (int j = 0; j < ROUTING_NUM; j++) {
                cout << mesh_topu_source[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;

        sleep(3); // 等待3秒
    }

    close(sock);
}

// -- 5g接收--
void recv_mesh_tuopu(const string& fiveG_ip, int recv_mesh_topu_source[ROUTING_NUM][ROUTING_NUM]) {
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

    while (true) {
        if (recvfrom(sock, recv_mesh_topu_source, ROUTING_NUM*ROUTING_NUM*sizeof(int), 0, (struct sockaddr *)&sender_addr, &sender_addr_len) < 0) {
            cerr << "Failed to receive data" << endl;
            close(sock);
            return;
        }

        cout << "Received data from " << inet_ntoa(sender_addr.sin_addr) << ":" << ntohs(sender_addr.sin_port) << endl;
        cout << "Received data:" << endl;
        for (int i = 0; i < ROUTING_NUM; i++) {
            for (int j = 0; j < ROUTING_NUM; j++) {
                cout << recv_mesh_topu_source[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }

    close(sock);
}



