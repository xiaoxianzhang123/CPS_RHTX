#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
// #include <net/if.h>


int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   /*核心操作，使用ioctl 的TUNSETIFF request实现创建/连接 ifr.name*/
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  /*
  如果成功，就把ifr.ifrname赋值给dev
  */
  strcpy(dev,ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
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
    static unsigned char buffer[2000];
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

