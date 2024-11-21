import socket
import time

# 发送端配置
server_ip = '127.0.0.1'    # 接收端IP地址
server_port = 12345        # 接收端端口号
total_data_size = 10240    # 10KB的数据
packet_size = 42           # 每个数据包的大小（小于典型的MTU）
send_interval = 1          # 发送间隔（秒）
num_sends = 10             # 发送次数

# 创建UDP套接字
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 准备10KB的数据
data = b'x' * total_data_size

# 获取当前时间作为开始时间
start_time = time.time()

count=0

while(1):
    current_time = time.time()
    # 检查是否到了下一个发送时间点
    if current_time - start_time >= send_interval:
        count+=1
        print(f"第{count}次发送...")
        if count>=11:break
        # 更新下一次发送的时间
        start_time = current_time

        # 将数据分割成多个包并发送
        for i in range(0, total_data_size, packet_size):
            end = min(i + packet_size, total_data_size)
            client_socket.sendto(data[i:end], (server_ip, server_port))

# 关闭套接字
client_socket.close()
