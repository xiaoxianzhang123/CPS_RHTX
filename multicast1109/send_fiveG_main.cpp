#include <iostream>
#include <string>
#include "multicast.h"

using namespace std;

// 图中的顶点数，后续根据SDK接口获取的节点数量来确定

int main(int argc, char* argv[]) {
    int fiveG_topu_source[ROUTING_NUM][ROUTING_NUM] = {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
        {21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
        {31, 32, 33, 34, 35, 36, 37, 38, 39, 40},
        {41, 42, 43, 44, 45, 46, 47, 48, 49, 50},
        {51, 52, 53, 54, 55, 56, 57, 58, 59, 60},
        {61, 62, 63, 64, 65, 66, 67, 68, 69, 70},
        {71, 72, 73, 74, 75, 76, 77, 78, 79, 80},
        {81, 82, 83, 84, 85, 86, 87, 88, 89, 90},
        {91, 92, 93, 94, 95, 96, 97, 98, 99, 100}
    };
    string mesh_ip = "192.168.16.128";
    string multicast_ip = "239.0.125.1";

    int num = send_fiveG_tuopu(mesh_ip, multicast_ip, fiveG_topu_source);
    cout << num << endl;

    return 0;
}