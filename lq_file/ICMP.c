#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#define PACKET_SIZE 64

// ICMP报文结构
struct icmp_packet {
    struct icmphdr header;
    char data[PACKET_SIZE - sizeof(struct icmphdr)];
};

int main(int argc, char *argv[]) {
    if (argc != 2) {//name is included
        fprintf(stderr, "Usage: %s <destination_ip>\n", argv[0]);
        exit(1);
    }
const char *dest_ip = argv[1];
struct sockaddr_in dest_addr;
int sockfd;


// 创建原始套接字
if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {//SOCK_RAW
    perror("socket");
    exit(1);
}


// 设置目标地址信息
memset(&dest_addr, 0, sizeof(dest_addr));
dest_addr.sin_family = AF_INET;
if (inet_pton(AF_INET, dest_ip, &(dest_addr.sin_addr)) <= 0) {
    perror("inet_pton");
    close(sockfd);
    exit(1);
}


// 构造ICMP Echo请求报文
struct icmp_packet packet;
memset(&packet, 0, sizeof(packet));
packet.header.type = ICMP_ECHO;
packet.header.code = 0;
packet.header.un.echo.id = getpid();
packet.header.un.echo.sequence=1;

// 获取发送时间
struct timeval send_time;
gettimeofday(&send_time, NULL);

// 发送ICMP Echo请求
if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
    perror("sendto");
    close(sockfd);
    exit(1);
}
printf("send is ok\n");

// 接收ICMP Echo响应
struct icmp_packet recv_packet;
socklen_t addr_len = sizeof(dest_addr);
if (recvfrom(sockfd, &recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&dest_addr, &addr_len) < 0) {
    perror("recvfrom");
    close(sockfd);
    exit(1);
}
printf("no?\n");

// 获取接收时间
struct timeval recv_time;
gettimeofday(&recv_time, NULL);

// 计算往返时间（RTT）
long rtt = (recv_time.tv_sec - send_time.tv_sec) * 1000 + (recv_time.tv_usec - send_time.tv_usec) / 1000;

printf("Received ICMP Echo Response from %s, RTT: %ld ms\n", dest_ip, rtt);

// 关闭套接字
close(sockfd);

return 0;
}
