#ifndef MULTICAST_H
#define MULTICAST_H

#include <string>

// 图中的顶点数，后续根据SDK接口获取的节点数量来确定
// #define ROUTING_NUM 10
#define MESH_NODE_NUM 32
#define FIVEG_NODE_NUM 16

//函数声名
// mesh向mesh发 发的是5g拓扑
int send_fiveG_tuopu(const std::string& mesh_ip, const std::string& multicast_ip, int fiveG_topu_source[FIVEG_NODE_NUM][FIVEG_NODE_NUM/4]); //   32 8 无非是只存16 *4
int recv_fiveG_tuopu(const std::string& mesh_ip, const std::string& multicast_ip);
//int recv_fiveG_tuopu(const std::string& mesh_ip, const std::string& multicast_ip, int fiveG_tuopu_source[MESH_NODE_NUM][MESH_NODE_NUM/4]);

// 5g向5g发 发的是mesh拓扑
void send_mesh_tuopu(const std::string& fiveG_ip1, const std::string& fiveG_ip2, int mesh_topu_source[MESH_NODE_NUM][MESH_NODE_NUM/4]);     // 32 8
void recv_mesh_tuopu();
// void recv_mesh_tuopu(const std::string& fiveG_ip);
//void recv_mesh_tuopu(const std::string& fiveG_ip, int mesh_signal_strength[FIVEG_NODE_NUM][FIVEG_NODE_NUM/4]);

// 5g向5g发 发的是5g拓扑

void fiveG_send_fiveG_tuopu(const std::string& fiveG_ip1, const std::string& fiveG_ip2,int fiveG_topu_source[FIVEG_NODE_NUM][FIVEG_NODE_NUM/4]);
void fiveG_recv_fiveG_tuopu();
// void fiveG_recv_fiveG_tuopu(const std::string& fiveG_ip);

// mesh向mesh发 发的是mesh拓扑 先不做 
#endif  // MULTICAST_H

