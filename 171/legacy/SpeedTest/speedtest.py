import socket
import time


testtime=5
speed=5



def recv():
    #接收端配置
    global testtime,speed
    server_ip = "127.0.0.1"   # 服务器IP地址
    server_port = 12345       # 服务器端口号
    buffer_size = 1024        # 接收缓冲区大小
    expected_size = testtime * speed * 1024 # 预期接收的总数据大小（10KB）

    # 创建UDP套接字
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 绑定套接字到地址
    server_socket.bind((server_ip, server_port))

    print("UDP Server up and listening")

    # 开始接收数据
    total_bytes_received = 0

    i=0
    total_time=0
    while total_bytes_received < expected_size :

        if i>=1:
            current_time=time.time()
            total_time=current_time-start_time
            if current_time-last_time>=1:
                last_time=current_time
                print(f"过去了{total_time}s,收到了{total_bytes_received}B数据")
            if  total_time >=testtime:break

        message, address = server_socket.recvfrom(buffer_size)
        total_bytes_received += len(message)

        i+=1
        if i==1 : 
            start_time = time.time()
            last_time=start_time
            print("开始计时")
        
        


        

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


def send():
    # 发送端配置
    global tesettime,speed
    server_ip = "127.0.0.1"    # 接收端IP地址
    server_port = 12345        # 接收端端口号
    total_data_size = 1024*speed    # 10KB的数据
    packet_size = 42           # 每个数据包的大小（小于典型的MTU）
    send_interval = 1          # 发送间隔（秒）


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
            if count>=testtime+1:break
            # 更新下一次发送的时间
            start_time = current_time

            # 将数据分割成多个包并发送
            for i in range(0, total_data_size, packet_size):
                end = min(i + packet_size, total_data_size)
                client_socket.sendto(data[i:end], (server_ip, server_port))

    # 关闭套接字
    client_socket.close()

choice=int(input("输入数字，1发送2接收\n"))
if choice==1:send()
elif choice==2:recv()
else: print("invalid choice")