import socket

def receive_file(save_as_filename, listen_ip, listen_port):
    # 创建UDP套接字
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 绑定IP和端口
    udp_socket.bind((listen_ip, listen_port))

    print(f"Listening on {listen_ip}:{listen_port}")

    # 打开要保存的文件
    with open(save_as_filename, 'wb') as file:
        while True:
            # 接收数据
            data, addr = udp_socket.recvfrom(1024)
            
            # 数据为空表示传输结束
            if not data:
                break
            
            # 写入数据到文件
            file.write(data)

    print("File received successfully.")

    # 关闭套接字
    udp_socket.close()

if __name__ == "__main__":
    file_to_save = "filetest.txt"
    listen_ip_address = "0.0.0.0"  # 监听所有网络接口
    listen_port = 12345  # 监听的端口号

    receive_file(file_to_save, listen_ip_address, listen_port)
