#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "mylib_tap.h"
#include "mylib_serialport.h"
#include "mylib_slip.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))



typedef struct {
    int serial_port;
    int tap_fd;
} thread_args;


void* sendd(void *args){
    thread_args *actual_args = (thread_args *)args;
    int serial_port = actual_args->serial_port;
    int tap_fd = actual_args->tap_fd;

    unsigned char tapframe[1600]={0};
    unsigned int tapframe_length=0;
    

    unsigned char serialframe[1600]={0};


    tapframe_length=get_tap_frame(tap_fd,tapframe);


    //组装串口帧，也就是加头加尾
    //serialframe就是最终获得的帧，长度就是tapframe_length+1+2+2+2+1=tapframe_length+8

    int encoded_data_length=0;
    encoded_data_length=generateframe(serialframe,tapframe,tapframe_length);

    printf("frame generated:\n");
    for(int i=0;i<encoded_data_length;i++){
        printf("%02x ",serialframe[i]);
    }
    printf("\n");

    write(serial_port, serialframe, encoded_data_length);
    printf("\n\n\n\n");

}

void* recvd(void* args){
    thread_args *actual_args = (thread_args *)args;
    int serial_port = actual_args->serial_port;
    int tap_fd = actual_args->tap_fd;

    int serial_frame_length=0;
    unsigned char serial_frame[BUFFER_SIZE];

    unsigned char tap_frame[1600]={0};


    //进入死循环，包括：1.从串口读数据 2.解包数据 3.写到网卡中
    serial_frame_length=read_serial_frame(serial_port,serial_frame);
    if(serial_frame_length<0){
        printf("read error, exiting...\n");
        exit(-1);}
    int decoded_data_length=slip_decode(&serial_frame[1],serial_frame_length-2);
    memcpy(tap_frame,&serial_frame[5],decoded_data_length-6);
    
    
    if(DEBUG_FLAG){
        printf("\ntap_frame_length:%d\n",decoded_data_length-6);
        printf("tap frame:\n");
        for(int i=0;i<decoded_data_length-6;i++){
            printf("%02x ",tap_frame[i]);
        }
        printf("\n");
    }


    write_tap_frame(tap_fd, tap_frame,decoded_data_length-6);
    printf("\n\n\n\n\n");
}

int main() {
    // 初始化
    char serialport[] = "/dev/ttyS5";
    char tap[] = "tap0";
    printf("opening serialport: %s ...\n", serialport);
    int serial_port = serial_port_init(serialport); // serial_port 初始化，返回 fd
    if (serial_port < 0) {
        perror("opening serial port error:");
        exit(-1);
    }
    printf("opening nic: %s...\n", tap);
    int tap_fd = tap_init(tap); // tap 初始化, 返回 file descriptor
    if (tap_fd < 0) {
        perror("opening nic error:");
        exit(-1);
    }

    // 设置 select() 监视的文件描述符集合
    fd_set read_fds, write_fds;
    int max_fd = MAX(serial_port, tap_fd);

    unsigned char tap_frame[1600] = {0};
    unsigned char serial_frame[1600] = {0};
    int tap_frame_length, serial_frame_length;


    thread_args args;
    args.serial_port = serial_port;
    args.tap_fd = tap_fd;


    while (1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(serial_port, &read_fds);
        FD_SET(tap_fd, &read_fds);

        // 使用 select() 监视文件描述符
        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
            perror("select error");
            exit(-1);
        }

        // 检查 TAP 设备是否准备好读取
        if (FD_ISSET(tap_fd, &read_fds)) {
            sendd(&args);
            // 处理从 TAP 读取的数据
            // ...（处理逻辑）
        }

        // 检查串行端口是否准备好读取
        if (FD_ISSET(serial_port, &read_fds)) {
            recvd(&args);
            // 处理从串行端口读取的数据
            // ...（处理逻辑）
        }
    }

    close(serial_port);
    close(tap_fd);
    return 0;
}