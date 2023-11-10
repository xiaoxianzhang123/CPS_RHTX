#include <iostream>
#include <string>
#include "multicast.h"

using namespace std;

// 图中的顶点数，后续根据SDK接口获取的节点数量来确定

int main(int argc, char* argv[]) {
    int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM];
    string mesh_ip = "192.168.16.129";  //接收端的mesh地址 。
    string multicast_ip = "239.0.125.1";  //组播组的地址

    int num = recv_fiveG_tuopu(mesh_ip, multicast_ip, fiveG_topu_source);
    cout << num << endl;

    return 0;
}