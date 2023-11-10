# 使用方法
## 编译
```cpp
g++ multicast.cpp send_fiveG_main.cpp -o send_fiveG
g++ multicast.cpp recv_fiveG_main.cpp -o recv_fiveG


g++ multicast.cpp send_mesh_main.cpp -o send_mesh
g++ multicast.cpp recv_mesh_main.cpp -o recv_mesh


```
## 运行
```cpp
./send_fiveG
./recv_fiveG

// 发送mesh亦同    
```
# 文件目录
## multicast.cpp文件
### send_fiveG_tuopu函数 
#### 功能
Mesh电台  组播 5G拓扑 （**发送方运行）**
#### 参数：
`mesh_ip`表示此设备的mesh电台的ip地址。
`multicast_ip`表示组播组的IP地址。
`fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]`表示要传递的5g拓扑。

### recv_fiveG_tuopu函数 
#### 功能
Mesh电台  组播 5G拓扑 （**接收方运行）**
#### 参数：
`mesh_ip`表示此设备的mesh电台的ip地址。
`multicast_ip`表示组播组的IP地址。
`fiveG_topu_source[ROUTING_NUM][ROUTING_NUM]`表示传递的5g拓扑。

### send_mesh_tuopu函数 
#### 功能
5G设备  组播 mesh拓扑 （**发送方运行）**
#### 参数：
`fiveG_ip1`表示5g设备的ip地址。
`fiveG_ip2`表示要组播的5G设备IP（池）地址。 
`mesh_topu_source[ROUTING_NUM][ROUTING_NUM]`表示要传递的mesh拓扑。
### recv_mesh_tuopu函数 
#### 功能
5G设备  组播 mesh拓扑 （**接收方运行）**
#### 参数：
`fiveG_ip1`表示5g设备发送方的ip地址（**非此设备的5gIP地址**）。
`mesh_topu_source[ROUTING_NUM][ROUTING_NUM]`表示传递的mesh拓扑。
## multicast.h文件
包含组播函数的声明，以及一些宏定义的配置。
## xxx_xxx_main文件
主函数文件中，用来传递参数，执行组播函数功能。

