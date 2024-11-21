import socket
import os
import json
import hashlib
import time



def loadconfig():
    global config_data
    # 读取JSON配置文件
    print("加载配置文件...")
    with open('config.json', 'r') as file:
        config_data = json.load(file)


def writeconfig():
    global config_data
    # 写入配置文件
    with open('config.json', 'w') as configfile:
        json.dump(config_data, configfile, indent=4)



def generate_random_file(file_path, size_in_bytes):    
    with open(file_path, 'wb') as file:
        random_data = os.urandom(size_in_bytes)
        file.write(random_data)




def calculate_speed(recvddatalen,totaltime):
    print(f"用时{totaltime:2f}s",end='')
    speed_in_Bbs=recvddatalen/totaltime
    if speed_in_Bbs <= 1000:
        print(f"速度：{speed_in_Bbs:.2f}B/s")
    elif 1000< speed_in_Bbs <= 1000000:
        print(f"速度：{speed_in_Bbs/1000:.2f}KB/s")
    elif 1000000< speed_in_Bbs <= 1000000000:
        print(f"速度：{speed_in_Bbs/1000000:.2f}MB/s")




def send_file(filename, target_ip, target_port):
    # 创建UDP套接字
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sentdata=0
    # 打开要发送的文件
    with open(filename, 'rb') as file:
        data = file.read(1024)  # 每次读取1024字节数据
        while data:
            # 发送数据
            sentdata+=udp_socket.sendto(data, (target_ip, target_port))
            print(f"已发送{sentdata}字节",end='\r')
            data = file.read(1024)
    print("\n发送完成")
    udp_socket.sendto(b'', (target_ip, target_port))
    # 关闭套接字
    udp_socket.close()

def receive_file(save_as_filename, listen_ip, listen_port):
    # 创建UDP套接字
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 绑定IP和端口
    udp_socket.bind((listen_ip, listen_port))

    print(f"Listening on {listen_ip}:{listen_port}")

    recvddatalen=0
    while(True):
        try:
            # 打开要保存的文件
            with open(save_as_filename, 'wb') as file:
                print("开始接收，ctrl+c中断")
                i=0
                while True:
                    # 接收数据
                    data, addr = udp_socket.recvfrom(1024)
                    i+=1
                    if i==1:
                        starttime=time.time()
                        print("测速开始")
                    recvddatalen+=len(data)
                    print(f"已收到{recvddatalen}字节",end="\r")
                    # 数据为空表示传输结束
                    if not data:
                        print("\n收到结束标志，结束中")
                        endtime=time.time()
                        totaltime=endtime-starttime
                        calculate_speed(recvddatalen,totaltime)
                        break
                    
                    # 写入数据到文件
                    file.write(data)

            print("接收完成")
            md5_hash = calculate_file_md5(save_as_filename)
            print(f"MD5 Hash of {save_as_filename}: {md5_hash}")
            recvddatalen=0
            print("准备下一次接收...\n\n\n")
        except KeyboardInterrupt:
            # 关闭套接字
            udp_socket.close()



def calculate_file_md5(file_path, buffer_size=8192):
    md5_hash = hashlib.md5()

    with open(file_path, 'rb') as file:
        while True:
            chunk = file.read(buffer_size)
            if not chunk:
                break
            md5_hash.update(chunk)

    return md5_hash.hexdigest()





if __name__ == "__main__":

    global config_data
    loadconfig()

    info='''
    '''
    print()
    choice=int(input("输入数字，发送方还是接收方：\n1：接收方\n2：发送方\n\n"))
    
    if(choice==1):
        file_to_save = "recvdtestdata"
        listen_ip_address = "0.0.0.0"  # 监听所有网络接口
        listen_port = 12345  # 监听的端口号
        receive_file(file_to_save, listen_ip_address, listen_port)
    elif(choice==2):
        # Example: Generate a random file of 1 MB
        size_input=input("你要发送多少字节？")
        if size_input == "":
            size=config_data["filesize"]
            print(f"使用上一次的配置：{size}")
        else:
            size=int(size_input)
            config_data["filesize"]=size
            writeconfig()
        print("生成随机文件...")
        generate_random_file('testdata',size)
        print(f"文件保存为testdata")

        file_to_send = "testdata"
        target_ip_address = input("输入对方ip:")
        if target_ip_address == "":
            print(f"使用上一次的配置：{config_data['serverip']}")
            target_ip_address=config_data["serverip"]
        else:
            config_data["serverip"]=target_ip_address
            writeconfig()

        # target_ip_address = "192.168.3.2"  # 修改为接收方的IP地址
        target_port = 12345  # 修改为接收方的端口号

        send_file(file_to_send, target_ip_address, target_port)
        file_path = 'testdata'
        md5_hash = calculate_file_md5(file_path)
        print(f"MD5 Hash of {file_path}: {md5_hash}")
    else: print("invalid choice")