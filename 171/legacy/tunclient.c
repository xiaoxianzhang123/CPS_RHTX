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



int main(){
    int tap_fd=tap_init();//初始化
    
    unsigned char tap_frame[1600]={0};
    int nread=0;
    nread=get_tap_frame(tap_fd,tap_frame);

}