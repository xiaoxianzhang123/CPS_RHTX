#ifndef MULTICAST_H
#define MULTICAST_H

#include <string>

// 图中的顶点数，后续根据SDK接口获取的节点数量来确定
#define ROUTING_NUM 10

// 函数声明
int send_fiveG_tuopu(const std::string& mesh_ip, const std::string& multicast_ip, int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]);
int recv_fiveG_tuopu(const std::string& mesh_ip, const std::string& multicast_ip, int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]);
// int recv_fiveG_tuopu2(const std::string& mesh_ip, const std::string& multicast_ip, int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]);

// ------------------
void send_mesh_tuopu(const std::string& fiveG_ip1, const std::string& fiveG_ip2, int mesh_topu_source[ROUTING_NUM][ROUTING_NUM]);
void recv_mesh_tuopu(const std::string& fiveG_ip, int recv_mesh_topu_source[ROUTING_NUM][ROUTING_NUM]);
#endif  // MULTICAST_H