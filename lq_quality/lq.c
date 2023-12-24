#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <math.h>
#include <time.h>
//new
#include <fcntl.h> // 添加头文件以支持非阻塞模式

#define PACKET_SIZE     4096
// #define MAX_WAIT_TIME   5
// #define MAX_NO_PACKETS  3
#define MAX_NSEND 65535
#define N 5//根据过去60次数据计算丢包率、时延波动
#define INTERVAL 1
//new
// #define EAGAIN 11
// #define EINTR 4 
// #define EWOULDBLOCK EAGAIN

char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
int sockfd,datalen=56;
int nsend=0,nreceived=0;


struct sockaddr_in dest_addr;
struct LinkQuality
{
    float onedelay;// 每INTERVAL测量一次，可以根据需要调整,单位为ms
    float standard_deviation;//-999表示过去N次没有收到任何包
    float loss_rate;//每次收到包就进行计算。
    float averagedelay;//past N 次.
    float multidelay[N];//循环数组
    int received;//过去N次总共收到的包
}lq;
int cir_i=0;//循环数组指针
_Bool sd_ok=0;//程序开始到现在是否获得了N次数据（用来计算标准差）


pid_t pid;
struct sockaddr_in from;
struct timeval tvrecv;
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pack_no);
void send_packet(void);
void recv_packet(void);
int unpack(char *buf,int len);
void tv_sub(struct timeval *out,struct timeval *in);
void statistics()
{       
    printf("\n--------------------LinkQuality statistics-------------------\n");
    printf("Delay is %f\n",lq.onedelay);
printf("%d\n",nsend);
    if(sd_ok==0) return;
    // printf("In past %d packets, %d received , %%%d lost\n",nreceived,(nsend-nreceived)/nsend*100);
    printf("In past %d packets,average delay is %f ,%d received , loss rate is %f,flutuate is %f\n",N,lq.averagedelay,lq.received,lq.loss_rate,lq.standard_deviation);
    
    
}
//new
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}


/*校验和算法*/
unsigned short cal_chksum(unsigned short *addr,int len)
{       int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;
		
/*把ICMP报头二进制数据以2字节为单位累加起来*/
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
		/*若ICMP报头为奇数个字节，会剩下后一字节。把后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
/*设置ICMP报头*/
int pack(int pack_no)
{       int i,packsize;
        struct icmp *icmp;
        struct timeval *tval;
        icmp=(struct icmp*)sendpacket;
        icmp->icmp_type=ICMP_ECHO;
        icmp->icmp_code=0;
        icmp->icmp_cksum=0;
        icmp->icmp_seq=pack_no;
        icmp->icmp_id=pid;
        printf("icmp->icmp_id is %d,and 2tid[count] is %d\n",icmp->icmp_id,pid);
        packsize=8+datalen;
        tval= (struct timeval *)icmp->icmp_data;
        gettimeofday(tval,NULL);    /*记录发送时间*/
        icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); /*校验算法*/
        return packsize;
}

// /*发送三个ICMP报文*/
// void send_packet()
// {       int packetsize;
//         while( nsend<MAX_NO_PACKETS)
//         {       nsend++;
//                 packetsize=pack(nsend); /*设置ICMP报头*/
//                 if( sendto(sockfd,sendpacket,packetsize,0,
//                           (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0  )
//                 {       perror("sendto error");
//                         continue;
//                 }
//                 //sleep(1); /*每隔一秒发送一个ICMP报文*/
//         }
// }

// void send_packet()
// {       int packetsize;
//         nsend=(nsend+1)%MAX_NSEND;
//         packetsize=pack(nsend); /*设置ICMP报头*/
//         if( sendto(sockfd,sendpacket,packetsize,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0  )
//         {       
//             perror("sendto error");
//         }
// }

// 修改发送数据的函数以支持非阻塞模式
void send_packet() {
    int packetsize;
    nsend = (nsend + 1) % MAX_NSEND;
    packetsize = pack(nsend);//并且进行计时
    if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {//是否要加一个循环？
            perror("sendto error");
        }
    }

    // //new 备用方案
    // int sendBytes = 0, recvBytes = 0, ret, offset = 0;
    // while (sendBytes < packetsize)
    //     {
    //         ret = sendto(sockfd, sendpacket+offset, packetsize-sendBytes, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    //         printf("ret==%d\n",ret);
    //         if (ret == 0) // 向服务端发送请求报文。
    //         { 
    //             perror("send"); 
    //             break; 
    //         }else if (ret == -1){
    //             if (errno == EWOULDBLOCK || errno == EAGAIN)
    //             {
    //                 // std::cout << "send kernal buffer full, retrying..." << std::endl;
    //                 printf("send kernal buffer full, retrying...\n");
    //                 continue;
    //             } else {
    //                 // 连接出错，关闭sockfd
    //                 close(sockfd);
    //                 return ;
    //             }
    //         }
    //         else
    //         {
    //             sendBytes += ret;
    //             offset += ret;
    //             // printf("已发送：%d 字节的数据\n", ret);
    //         }
    //     }

}

// /*接收所有ICMP报文*/
// void recv_packet()
// {       int n,fromlen;
//         extern int errno;
//         signal(SIGALRM,statistics);
//         fromlen=sizeof(from);
//         while( nreceived<nsend)
//         {       alarm(MAX_WAIT_TIME);
//                 if( (n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,
//                                 (struct sockaddr *)&from,&fromlen)) <0)
//                 {       if(errno==EINTR)continue;
//                         perror("recvfrom error");
//                         continue;
//                 }
//                 gettimeofday(&tvrecv,NULL);  /*记录接收时间*/
//                 if(unpack(recvpacket,n)==-1)continue;
//                 nreceived++;
//         }
// }



// void recv_packet()
// {       
//         int n,fromlen;
//         extern int errno;
//         // signal(SIGALRM,statistics);
//         // alarm(MAX_WAIT_TIME);
//         int received=0;
//         fromlen=sizeof(from);
//         struct timeval start_time, end_time;
//         double elapsed_time;
//         int outdate=0;
//         gettimeofday(&start_time, NULL);
//         while(1)
//         {
//             gettimeofday(&end_time, NULL);
//             elapsed_time = (end_time.tv_sec - start_time.tv_sec) +   (end_time.tv_usec - start_time.tv_usec) / 1e6;
//             if(elapsed_time>=5.0)
//             {
//                 break;
//             }
//             if( (n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr *)&from,&fromlen)) <0)
//             {       
//                 if(errno==EINTR)continue;
//                 // perror("recvfrom error");
//                 usleep(1) //1 1.7跳跃 并且延迟也相近  不加在67 100跳跃  sleep(1)会休息1s太慢了
//                 
//                 //sleep(1);
//             }
//             // printf("n==%d\n",n);//debug
//             gettimeofday(&tvrecv,NULL);  /*记录接收时间*/
//             if(unpack(recvpacket,n)==-1)continue;//如果收到的包是上次的，则重新接收包。
//             else {received=1;break;}//如果是这次的，就跳出循环   
//         }
//         if(received==1)
//         {nreceived++;}
//         else
//         {
//             lq.onedelay=-1;//表示无法收到包
//             lq.multidelay[cir_i]=-1;
//             cir_i=(cir_i+1)%N;
//             printf("ok\n");
//         } 
// }

void recv_packet()
{       
        int n,fromlen;
        extern int errno;
        int received=0;
        fromlen=sizeof(from);
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 5;// 设置最大等待时间为5s
        timeout.tv_usec = 0; 
        while(1)//防止收到以前的响应包
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            // printf("ret=%d\n",ret);
            if (ret == 0) 
            {
            // Timeout, no data available, continue waiting
                break;
            }
            else if (ret < 0) 
            {
                // Error, handle error as needed
                perror("select error");
                break;
            }
            else if(ret > 0)
            {
                if (FD_ISSET(sockfd, &readfds)) 
                {
                    memset(recvpacket,0,sizeof(recvpacket));
                    if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen)) < 0) {
                        if (errno == EINTR) {
                            continue;
                        }
                        perror("recvfrom error");
                    } 
                    else 
                    {
                        gettimeofday(&tvrecv, NULL);
                        if (unpack(recvpacket, n) == -1) //如果收到的包是上次的，则重新接收包。
                        {
                            printf("hello1\n");
                            continue;
                        } 
                        else //如果是这次的，就跳出循环 
                        {
                            received = 1;
                            break;
                        }
                    }
                }
            }
        }
        if(received==1)
        {nreceived++;}
        else
        {
            lq.onedelay=-1;//表示无法收到包
            lq.multidelay[cir_i]=-1;
            cir_i=(cir_i+1)%N;
            // printf("ok\n");
        } 
}

// // 修改接收数据的函数以支持非阻塞模式 不知道收到的ICMP的大小
// void recv_packet() {
//     int n, fromlen;
//     extern int errno;
//     int received = 0;
//     fromlen = sizeof(from);
//     struct timeval start_time, end_time;
//     double elapsed_time;
//     int outdate=0;
//     gettimeofday(&start_time, NULL);
//     int recvBytes = 0,  ret=0, offset = 0;
    
//     while(1)
//     {
//         while (recvBytes < 84)
//         {
//             gettimeofday(&end_time, NULL);
//             elapsed_time = (end_time.tv_sec - start_time.tv_sec) +   (end_time.tv_usec - start_time.tv_usec) / 1e6;
//             if(elapsed_time>=5.0)
//             {
//                 outdate=1;
//                 break;
//             }
//             ret = recvfrom(sockfd, recvpacket+offset, sizeof(recvpacket)-recvBytes, 0, (struct sockaddr *)&from, &fromlen);
//             printf("ret==%d\n",ret);//debug
//             if (ret == 0) // 向服务端发送请求报文。
//             { 
//                 perror("recv"); 
//                 printf("other end close\n");
//                 // std::cout << "other end close"<< std::endl;
//                 break; 
//             }else if (ret == -1){
//                 if (errno == EWOULDBLOCK || errno == EAGAIN)
//                 {
//                     // std::cout << "recv kernal buffer full, retrying..." << std::endl;
//                     //printf("recv kernal buffer full, retrying...\n");
//                     // perror("recv"); 
//                     // sleep(2);
//                     continue;
//                 } else {
//                     // 连接出错，关闭connfd
//                     close(sockfd);
//                     return ;
//                 }
//             }
//             else
//             {
//                 recvBytes += ret;
//                 offset += ret;
//                 // printf("从服务端接收：%d 字节的数据\n", ret);
//             }
//         }
//         // printf("1\n");
//         if(outdate)
//         {
//             break;
//         }
//         gettimeofday(&tvrecv, NULL);
//         // printf("2\n");
//         if (unpack(recvpacket, n) == -1) 
//         {
//             printf("no\n");
//             continue;
//         }
//         else 
//         {
//             received = 1;
//             break;
//         }

//     }
//     printf("1\n");
//     if (received == 1) {
//         nreceived++;
//     } else {
//         lq.onedelay = -1;
//         lq.multidelay[cir_i] = -1;
//         cir_i = (cir_i + 1) % N;
//         printf("ok\n");
//     }
// }

/*剥去ICMP报头*/
int unpack(char *buf,int len)
{       int i,iphdrlen;
        struct ip *ip;
        struct icmp *icmp;
        struct timeval *tvsend;
        double rtt;
        ip=(struct ip *)buf;
        iphdrlen=ip->ip_hl<<2;    /*求ip报头长度,即ip报头的长度标志乘4*/
        icmp=(struct icmp *)(buf+iphdrlen);  /*越过ip报头,指向ICMP报头*/
        len-=iphdrlen;            /*ICMP报头及ICMP数据报的总长度*/
        if( len<8)                /*小于ICMP报头长度则不合理*/
        {       //printf("ICMP packets\'s length is less than 8\n");
                return -1;
        }
        printf("buf is %d\n",icmp->icmp_type);
        /*确保所接收的是我所发的的ICMP的回应*/
        if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
        {       tvsend=(struct timeval *)icmp->icmp_data;
                tv_sub(&tvrecv,tvsend);  /*接收和发送的时间差*/
                rtt=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000;  /*以毫秒为单位计算rtt*/
                /*显示相关信息*/
                lq.onedelay=rtt;
                lq.multidelay[cir_i]=rtt;
                cir_i=(cir_i+1)%N;
                // printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                //         len,
                //         inet_ntoa(from.sin_addr),
                //         icmp->icmp_seq,
                //         ip->ip_ttl,
                //         rtt);
        }
        else    return -1;//
}

void cal_standard_deviation()
{
    float sum=0;
    float average=0;
    int i;
    int n=N;//参与计算的总样本
    for(i=0;i<=N-1;i++)
    {
        if(lq.multidelay[i]==-1) 
        {
            n--;
            continue;
        }
        sum+=lq.multidelay[i];
    }
    if(n==0) 
    {
        lq.averagedelay=-1;
        lq.standard_deviation=-999;
        return;
    }
    average=sum/n;
    lq.averagedelay=average;
    float mean_sum=0;
    for(i=0;i<=N-1;i++)
    {
        if(lq.multidelay[i]==-1) continue;
        mean_sum+=(average-lq.multidelay[i])*(average-lq.multidelay[i]);
    }
    lq.standard_deviation=sqrt(mean_sum/(n-1));

}

void cal_loss_rate()
{
    int i;
    int loss_num=0;
    for(i=0;i<=N-1;i++)
    {
        if(lq.multidelay[i]==-1)
        {
            loss_num++;
        }
    }
    lq.received=N-loss_num;
    lq.loss_rate=(float)loss_num/N;
}

void linkquality()
{
        while(1)
        {
            send_packet();
            recv_packet();
            if(nsend>=N)
            {
                sd_ok=1;
            }
            if(sd_ok)
            {
                cal_standard_deviation();
            }
            if(sd_ok)
            {
                cal_loss_rate();
            }
            statistics(); /*进行统计*/
            sleep(INTERVAL);

        }

        
}

int main(int argc,char *argv[])
{       
    const char *dest_ip = argv[1];
    struct hostent *host;
    struct protoent *protocol;
    // int waittime=MAX_WAIT_TIME;
    int size=50*1024;
    if(argc<2)
    {       printf("usage:%s IP address\n",argv[0]);
            exit(1);
    }
    if( (protocol=getprotobyname("icmp") )==NULL)
    {       perror("getprotobyname");
            exit(1);
    }
    /*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
    if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
    {       perror("socket error");
            exit(1);
    }
    //new

    // 设置套接字为非阻塞模式
    if (set_nonblocking(sockfd) != 0) {
        close(sockfd);
        exit(1);
    }
    
    /* 回收root权限,设置当前用户权限*/
    setuid(getuid());
    /*扩大套接字接收缓冲区到50K这样做主要为了减小接收缓冲区溢出的
        的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
    // 设置目标地址信息
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, dest_ip, &(dest_addr.sin_addr)) <= 0) 
    {
        perror("inet_pton");
        close(sockfd);
        exit(1);
    }
        
    /*获取main的进程id,用于设置ICMP的标志符*/
    pid=getpid();
    printf("LQ %s(%s): %d bytes data in ICMP packets.\n",argv[1],inet_ntoa(dest_addr.sin_addr),datalen);
    linkquality();
    close(sockfd);
    return 0;
}

/*两个timeval结构相减*/
void tv_sub(struct timeval *out,struct timeval *in)
{       if( (out->tv_usec-=in->tv_usec)<0)
        {       --out->tv_sec;
                out->tv_usec+=1000000;
        }
        out->tv_sec-=in->tv_sec;
}
/*------------- The End -----------*/


