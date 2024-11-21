import socket

def send_file(filename, target_ip, target_port):
    # 创建UDP套接字
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 打开要发送的文件
    with open(filename, 'rb') as file:
        data = file.read(1024)  # 每次读取1024字节数据
        while data:
            # 发送数据
            udp_socket.sendto(data, (target_ip, target_port))
            data = file.read(1024)

    # 关闭套接字
    udp_socket.close()

if __name__ == "__main__":
    file_to_send = "testdata"
    target_ip_address = "192.168.3.2"  # 修改为接收方的IP地址
    target_port = 12345  # 修改为接收方的端口号

    send_file(file_to_send, target_ip_address, target_port)
