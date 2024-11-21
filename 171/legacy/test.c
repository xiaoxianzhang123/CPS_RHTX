#include<stdio.h>
#include <string.h>
#include <stdlib.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "tunclient.h"
#include <unistd.h>
#include <stdlib.h>
// #include <net/if.h>



unsigned short getCRC(unsigned char* buf, unsigned int len){

    unsigned int i,j;
    unsigned short crc,flag;
    crc=0x0000;
    for(i=0;i<len;i++){
        crc^=(((unsigned short)buf[i])<<8);
        for (j=0;j<8;j++){
            flag=crc&0x8000;
            crc<<=1;
            if(flag){
                crc&=0xfffe;
                crc^=0x8005;
            }
        }
    }
    return crc;
}

char* modifyArray(unsigned char *buffer, size_t length) {
    // 创建新数组，长度为原数组长度加上固定头部和长度信息的长度
    size_t newLength = length + 4; // 2 bytes for fixed header + 2 bytes for length
    unsigned char *newBuffer = (unsigned char *)malloc(newLength);

    // 添加固定开头 {0x55, 0x10}
    newBuffer[0] = 0x55;
    newBuffer[1] = 0x00;

    // 添加数组长度，两个字节
    newBuffer[2] = (unsigned char)(length >> 8); // 高字节
    newBuffer[3] = (unsigned char)length;        // 低字节

    // 使用memcpy复制原数组内容
    memcpy(newBuffer + 4, buffer, length);

    // 打印结果，这里可以根据需要进行其他操作
    printf("Modified Array: { ");
    for (size_t i = 0; i < newLength; ++i) {
        printf("0x%02X ", newBuffer[i]);
    }
    printf("}\n");

    // 释放动态分配的内存
    // free(newBuffer);
    return newBuffer;
}

// void get_tap_frame(char* tapframe, unsigned int* tapdata_length){
//     //get data and len from tap:
//     // get_data_and_len()
//     static unsigned char inputStr[1600];
//     memset(inputStr,0,sizeof(inputStr));
//     printf("请输入一串字符串：");
//     fgets(inputStr, sizeof(inputStr), stdin); // 使用fgets读取包括空格的字符串
//     unsigned char* frame;
//     frame=inputStr;
//     // unsigned char buffer[] = {0x01, 0x02, 0x33};
//     // size_t length = sizeof(buffer) / sizeof(buffer[0]);
//     size_t length=strlen(frame)-1;


//     memcpy(tapframe,frame,length);
//     *tapdata_length=length;
//     printf("got date from tap:datalength:%d",length);
// }




void generateframe(char* serialframe,char* tapframe,unsigned int tapdata_length){
    serialframe[0]=0xc0;
    serialframe[tapdata_length+1+4+2]=0xc0;

    char *newframe=0;
    newframe=modifyArray(tapframe,tapdata_length);//获得加上头部的帧

    memcpy(&serialframe[1],newframe,tapdata_length+6);

    unsigned short crc=0;
    crc=getCRC(newframe,tapdata_length+4);
    printf("crc:%x",crc);
    serialframe[tapdata_length+1+4]=(unsigned char)(crc >> 8);
    serialframe[tapdata_length+1+4+1]=(unsigned char)crc;

    free(newframe);
}



int tap_init(){
    char dev_name[IFNAMSIZ]="tap0";
    int tap_fd=0;

    tap_fd=tun_alloc(dev_name,IFF_TAP);
    if(tap_fd < 0){
      perror("Allocating error");
      exit(1);
    }
  
    return tap_fd;
}


int get_tap_frame(int tap_fd,char* tap_frame){
    static buffer[2000];
    memset(buffer,0,sizeof(buffer));
    int nread=0;

    nread=read(tap_fd,buffer,sizeof(buffer));
    if(nread < 0){
      perror("reading from interface");
      close(tap_fd);
      exit(1);
    }

    printf("read %d bytes data:\n",nread);
    // printf("%0*x\n",nread,buffer);
    for (int i = 0; i < nread; i++) {
      printf("%02x ", buffer[i]);
    }
    printf("\n");

    memcpy(tap_frame,buffer,nread);
    return nread;

}



void wirte_tap_frame(char* tapframe,int tapframe_length){
    
}
int main() {
    
    //读网卡，返回数据和数据长度
    unsigned char tapframe[1600]={0};
    unsigned int tapframe_length=0;
    

    int tap_fd=tap_init();//初始化
    
    tapframe_length=get_tap_frame(tap_fd,tapframe);

    //组装串口帧，也就是加头加尾
    //serialframe就是最终获得的帧，长度就是tapdata_length+1+2+2+2+1=tapdata_length+8
    unsigned char serialframe[1600]={0};
    generateframe(serialframe,tapframe,tapframe_length);

    printf("frame generated:\n");
    for(int i=0;i<tapframe_length+8;i++){
        printf("%02x ",serialframe[i]);
    }


    wirte_tap_frame(tapframe,tapframe_length);

    return 0;

    
    
    // char addhead_frame[1600]={0};
    // char* dynamic=modifyArray(frame,length);
    // memcpy(addhead_frame,modifyArray(frame,length),length+4);
    // free(dynamic);
    // //add crc
    // unsigned short crc=getCRC(addhead_frame,length+4);
    // printf("%x\n",crc);
    // unsigned char crc_high=(unsigned char)(crc>>8);
    // unsigned char crc_low=(unsigned char)crc;
    // printf("the crc is:%02x %02x",crc_high,crc_low);

    // return 0;


    // read(tun)
    // head getCRC
    // write(serial)


    // getframe()
    // write(tun)


    // return 0;
}


// int main() {
//     char inputStr[1600]={};
//     printf("请输入一串字符串：");
//     fgets(inputStr, sizeof(inputStr), stdin); // 使用fgets读取包括空格的字符串



//     size_t len = strlen(inputStr);
//     if (inputStr[len - 1] == '\n') {
//         inputStr[--len] = '\0';
//     }


    //inputStr,lenstr
    
    // 输出最终的字节序列，格式为16进制
    // printf("输出的16进制格式字符串为：{");
    // for (size_t i = 0; i < len + 4; i++) {
    //     printf("0x%02X", outputBuffer[i]); // 以16进制格式输出
    //     if (i < len + 4 - 1) {
    //         printf(", ");
    //     }
    // }
    // printf("}\n");

    
//     return 0;
// }


// int main(){
//     /*
//     \xc0\x55\x00\x00\x08\x01\x00\x0e\x31\xb9\x56\x34\x2a\x6a
//     */
//     unsigned char buf[]={0x55,0x00,0x00,0x01,0x30};
//     unsigned int len=sizeof(buf);

    
//     printf("%x",getCRC(buf,len));
// }

// \xc0\x55\x00\x00\x01\x00\x9e\x1b\xc0

// echo -e "\xc0\x55\x00\x00\x01\x00\x9e\x1b\xc0" >/dev/ttyS5
// echo  -e "\xc0\x55\x00\x00\x08\x01\x00\x0e\x31\xb9\x56\x34\x2a\x6a\x98\xc0\r\n" >/dev/ttyS5