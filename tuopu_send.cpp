#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MCAST_PORT 8888
#define MCAST_ADDR "239.0.0.1" // 组播地址
#define BUF_SIZE 1024
#define V 8
#define routing_num 8
#define strength_num 2 //D1 D2


int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in mcast_addr;
    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);
    mcast_addr.sin_port = htons(MCAST_PORT);

    while (true) {
          int mesh_signal_strength[routing_num][strength_num] = {
        {12, 0},
        {227, 0},
        {8, 2},
        {12, 8},
        {32, 8},
        {128, 2},
        {0, 0},
        {0, 0}
    };
    
    int fiveG_signal_strength[routing_num][strength_num] = {
        {12, 0},
        {131, 0},
        {0, 2},
        {8, 0},
        {32, 0},
        {0, 0},
        {0, 0},
        {0, 0}
    };

        sendto(sock, fiveG_signal_strength, sizeof(fiveG_signal_strength), 0,
               (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
        sendto(sock, mesh_signal_strength, sizeof(mesh_signal_strength), 0,
               (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));

        sleep(3);
    }

    close(sock);
    return 0;
}