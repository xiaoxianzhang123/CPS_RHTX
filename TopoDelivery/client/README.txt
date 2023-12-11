编译命令：g++ multicast.cpp routing_broadcast.cpp -o code -pthread -I./ -I/usr/include/jsoncpp -lcurl -ljsoncpp
用法：./code <ID_Number> <5G_server_IP> <mesh_broadcast_IP> <my mesh_IP:192.168.x.9>
注意：没有5G或是没有连接自组网电台的融通器就输入0