#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MCAST_PORT 8888
#define MCAST_ADDR "239.0.0.1"
#define BUF_SIZE 1024
#define V 8
#define routing_num 8
#define strength_num 2 //D1 D2

struct Data {
    int fiveG_signal_strength[routing_num][strength_num];
    int mesh_signal_strength[routing_num][strength_num];
};

int main(int argc, char* argv[]) {
    if (argc != 2 * routing_num * strength_num + 1) {
        std::cerr << "Incorrect number of arguments!" << std::endl;
        return -1;
    }

    Data data;
    int index = 1;

    for (int i = 0; i < routing_num; i++) {
        for (int j = 0; j < strength_num; j++) {
            data.fiveG_signal_strength[i][j] = std::stoi(argv[index++]);
        }
    }

    for (int i = 0; i < routing_num; i++) {
        for (int j = 0; j < strength_num; j++) {
            data.mesh_signal_strength[i][j] = std::stoi(argv[index++]);
        }
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in mcast_addr;
    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);
    mcast_addr.sin_port = htons(MCAST_PORT);

    while (true) {
        sendto(sock, &data, sizeof(data), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
        sleep(3);
    }

    close(sock);
    return 0;
}