# CPS_RHTX
CPS实验室融合通信项目

# udp_multicast.cpp 
## 编译
`g++ -pthread -o udp_multicast udp_multicast.cp`
## 运行
`./udp_multicast`
![image](https://github.com/xiaoxianzhang123/CPS_RHTX/assets/85818041/74d34f95-e3ba-465f-95fa-b2ee2edd5893)
Dijkstra_shell_arg.cpp是目前更改到的版本

## tuopu_recv和 tuopu_send文件
无参传递<br>
`#define MCAST_PORT 8888`<br>
`#define MCAST_ADDR "239.0.0.1"`<br>
`#define BUF_SIZE 1024`<br>
`#define V 8`<br>
`#define routing_num 8`<br>
`#define strength_num 2 //D1 D2` <br>
## top_recv 和 top_send文件
 ./top_send 1 2 3 4 5 6 ...  // 参数为两个数组的值<br>
参数要两个参数的值。32个值。<br>
`#define MCAST_PORT 8888`<br>
`#define MCAST_ADDR "239.0.0.1"`<br>
`#define BUF_SIZE 1024`<br>
`#define V 8`<br>
`#define routing_num 8`<br>
`#define strength_num 2 //D1 D2` <br>
