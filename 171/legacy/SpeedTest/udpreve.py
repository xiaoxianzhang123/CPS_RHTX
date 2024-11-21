import socket
import time

# 接收端配置
server_ip = '127.0.0.1'   # 服务器IP地址
server_port = 12345       # 服务器端口号
buffer_size = 1024        # 接收缓冲区大小
expected_size = 100 * 1024 # 预期接收的总数据大小（10KB）

# 创建UDP套接字
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 绑定套接字到地址
server_socket.bind((server_ip, server_port))

print("UDP Server up and listening")

# 开始接收数据
total_bytes_received = 0

i=0
while total_bytes_received < expected_size:
    message, address = server_socket.recvfrom(buffer_size)
    i+=1
    if i==1 : 
        start_time = time.time()
        print("开始计时")
    total_bytes_received += len(message)

# 记录接收结束的时间
end_time = time.time()+1

# 计算接收速率
total_time = end_time - start_time
speed = total_bytes_received / total_time

print(f"Total bytes received: {total_bytes_received}")
print(f"Total time: {total_time:.2f} seconds")
print(f"Speed: {speed:.2f} bytes/second")

# 关闭套接字
server_socket.close()

