编译命令：g++ http_interfaces.cpp multicast.cpp routing_broadcast.cpp -o code -pthread -I./ -I/usr/include/jsoncpp -lcurl -ljsoncpp -lcpprest -lboost_system -lboost_chrono -lboost_thread -lssl -lcrypto
用法：./code <ID_Number>
编译:直接make