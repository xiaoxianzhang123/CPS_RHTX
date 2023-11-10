#include <iostream>
#include <string>
#include "multicast.h"

using namespace std;

// 图中的顶点数，后续根据SDK接口获取的节点数量来确定


int main() {
    int recv_mesh_topu_source[ROUTING_NUM][ROUTING_NUM];

    std::string fiveG_ip = "192.168.16.128"; // fiveG_ip2 地址

    recv_mesh_tuopu(fiveG_ip, recv_mesh_topu_source);

    
    return 0;
}