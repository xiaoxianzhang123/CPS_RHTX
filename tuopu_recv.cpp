#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MCAST_PORT 8888
#define MCAST_ADDR "239.0.253.0"
#define BUF_SIZE 1024
#define V 8
#define routing_num 8
#define strength_num 2 //D1 D2


int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(MCAST_PORT);

    bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    while (true) {
        int fiveG_signal_strength[routing_num][strength_num];
        int mesh_signal_strength[routing_num][strength_num];

        recvfrom(sock, fiveG_signal_strength, sizeof(fiveG_signal_strength), 0, NULL, NULL);
        recvfrom(sock, mesh_signal_strength, sizeof(mesh_signal_strength), 0, NULL, NULL);

        // 打印数组
       std::cout << "fiveG_signal_strength:" << std::endl;
        for (int i = 0; i < routing_num; i++) {
            for (int j = 0; j < strength_num; j++) {
                std::cout << fiveG_signal_strength[i][j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;

        std::cout << "mesh_signal_strength:" << std::endl;
        for (int i = 0; i < routing_num; i++) {
            for (int j = 0; j < strength_num; j++) {
                std::cout << mesh_signal_strength[i][j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------" << std::endl;
    }

    close(sock);
    return 0;
}
