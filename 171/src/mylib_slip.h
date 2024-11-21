#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// 编码函数，实现 SLIP 编码逻辑
int slip_encode(unsigned char* serial_frame, int length) {
    // 估算转义后的长度可能会增加，最坏情况下每个字符都需要转义
    int max_encoded_length = length * 2;
    char* buffer = (char*)malloc(max_encoded_length);
    int j = 0;
    for (int i = 0; i < length; ++i) {
        switch (serial_frame[i]) {
            case 0xC0:
                buffer[j++] = 0xDB;
                buffer[j++] = 0xDC;
                break;
            case 0xDB:
                buffer[j++] = 0xDB;
                buffer[j++] = 0xDD;
                break;
            default:
                buffer[j++] = serial_frame[i];
        }
    }

    // 复制 buffer 中的内容回原数组
    for (int i = 0; i < j; ++i) {
        serial_frame[i] = buffer[i];
    }

    // 释放中间缓冲区内存
    free(buffer);

    // 返回编码后的长度
    return j;
}

// 解码函数，实现 SLIP 解码逻辑
int slip_decode(unsigned char* tap_frame, int length) {
    int j = 0;
    for (int i = 0; i < length; ++i) {
        if (tap_frame[i] == 0xDB && i + 1 < length) {
            switch (tap_frame[i + 1]) {
                case 0xDC:
                    tap_frame[j++] = 0xC0;
                    i++; // 跳过转义后面的字符
                    break;
                case 0xDD:
                    tap_frame[j++] = 0xDB;
                    i++; // 跳过转义后面的字符
                    break;
                default:
                    tap_frame[j++] = tap_frame[i];
            }
        } else {
            tap_frame[j++] = tap_frame[i];
        }
    }
    return j; // 返回解码后的长度
}


unsigned char* modifyArray(unsigned char *buffer, size_t length) {
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
    printf("Modified Array:\n { ");
    for (size_t i = 0; i < newLength; ++i) {
        printf("0x%02X ", newBuffer[i]);
    }
    printf("}\n");

    // 释放动态分配的内存
    // free(newBuffer);
    return newBuffer;
}


int generateframe(unsigned char* serialframe,unsigned char* tapframe,size_t tapdata_length){
    serialframe[0]=0xc0;
    // serialframe[tapdata_length+1+4+2]=0xc0;

    unsigned char *newframe=0;
    newframe=modifyArray(tapframe,tapdata_length);//获得加上头部的帧

    memcpy(&serialframe[1],newframe,tapdata_length+6);

    unsigned short crc=0;
    crc=getCRC(newframe,tapdata_length+4);
    printf("crc:%x\n",crc);
    serialframe[tapdata_length+1+4]=(unsigned char)(crc >> 8);
    serialframe[tapdata_length+1+4+1]=(unsigned char)crc;

    int encoded_data_length=0;
    encoded_data_length=slip_encode(&serialframe[1],tapdata_length+6);


    serialframe[encoded_data_length+1]=0xc0;
    free(newframe);
    return encoded_data_length+2;
}