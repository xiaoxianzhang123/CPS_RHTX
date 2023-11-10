//目的：打印出5G设备是否在线的情况，如果再多输入参数-lq，则还会打印5G的链路质量。（结果会存在数组中）

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h> // 添加头文件以支持非阻塞模式
#include <math.h>

#define Node 3
#define N 5
#define PACKET_SIZE 4096

typedef struct{
    float onedelay;// 每INTERVAL测量一次，可以根据需要调整,单位为ms
    float standard_deviation;//-999表示过去N次没有收到任何包
    float loss_rate;//每次收到包就进行计算。
    float averagedelay;//past N 次.
    float multidelay[N];//循环数组
    int received;//过去N次总共收到的包
}LQ;

//存储online信息的指针，空间后续分配
int *online = NULL;
LQ *lq = NULL;

//地址
struct sockaddr_in dest_addr[Node+1];

//socket
int sockfd[Node+1];

int nsend[Node+1],nreceived[Node+1];
//char sendpacket[Node+1][PACKET_SIZE];
//char recvpacket[Node+1][PACKET_SIZE];
//pthread_t tid[Node+1];
int datalen=56;
struct sockaddr_in from[Node+1];//干嘛的
struct timeval tvrecv[Node+1];//干嘛的
int cir_i[Node+1];//计数

/*两个timeval结构相减*/
void tv_sub(struct timeval *out,struct timeval *in)
{       if( (out->tv_usec-=in->tv_usec)<0)
        {       --out->tv_sec;
                out->tv_usec+=1000000;
        }
        out->tv_sec-=in->tv_sec;
}

int set_nonblocking(int sockfd) 
{
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
int pack(int pack_no,int count,char *sendpacket)
{       
    int i,packsize;
    struct icmp *icmp;
    struct timeval *tval;
    icmp=(struct icmp*)sendpacket;
    icmp->icmp_type=ICMP_ECHO;
    icmp->icmp_code=0;
    icmp->icmp_cksum=0;
    icmp->icmp_seq=pack_no;
    //printf("icmp->icmp_id is %ld,and 3count is %ld\n",icmp->icmp_id,count);
    icmp->icmp_id=count;
    //printf("icmp->icmp_id is %d,and 2count is %d\n",icmp->icmp_id,count);
    packsize=8+datalen;
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);    /*记录发送时间*/
    icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); /*校验算法*/
    return packsize;
}

void send_packet(int count) 
{
    char sendpacket[PACKET_SIZE];
    int packetsize;
    nsend[count] = nsend[count] ++;
    packetsize = pack(nsend[count],count,sendpacket);//并且进行计时
    //printf("hello\n");
    if (sendto(sockfd[count], sendpacket, packetsize, 0, (struct sockaddr *)&dest_addr[count], sizeof(dest_addr[count])) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {//是否要加一个循环？
            perror("sendto error");
        }
    }
}

/*剥去ICMP报头*/
int unpack(char *buf,int len,int count,int flag)
{
    //printf("count is %d\n",count);
    int i,iphdrlen;
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
    //printf("buf is %d\n",icmp->icmp_type);
    /*确保所接收的是我所发的的ICMP的回应*/
    if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==count) )
    {       tvsend=(struct timeval *)icmp->icmp_data;
            tv_sub(&(tvrecv[count]),tvsend);  /*接收和发送的时间差*/
            rtt=tvrecv[count].tv_sec*1000+tvrecv[count].tv_usec/1000;  /*以毫秒为单位计算rtt*/
            /*显示相关信息*/
            if(flag==1)
            {
                lq[count].onedelay=rtt;
                lq[count].multidelay[cir_i[count]]=rtt;
                cir_i[count]++;
                // printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                //         len,
                //         inet_ntoa(from.sin_addr),
                //         icmp->icmp_seq,
                //         ip->ip_ttl,
                //         rtt);
            }

    }
    else
    {
        // if(icmp->icmp_type!=ICMP_ECHOREPLY)
        // {
        //     printf("hello2\n");//debug 输出这个,表明返回的包不是ICMP包？
        // }
        // if(icmp->icmp_id!=count)
        // {
        //     //printf("icmp->icmp_id is %d,and tid[count] is %d\n",icmp->icmp_id,count);//debug tid[count]的值变了
        //     printf("hello3\n");//debug 不输出这个
        // }
        return -1;//
    }    
}

void recv_packet(int count,int flag)
{       
        int n,fromlen;
        extern int errno;
        int received=0;
        fromlen=sizeof(from[count]);
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 5;// 设置最大等待时间为5s
        timeout.tv_usec = 0; 
        while(1)//防止收到以前的响应包
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd[count], &readfds);
            int ret = select(sockfd[count] + 1, &readfds, NULL, NULL, &timeout);
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
                //printf("hello\n");//debug 打印了多个hello
                if (FD_ISSET(sockfd[count], &readfds)) 
                {
                    char recvpacket[PACKET_SIZE];
                    if ((n = recvfrom(sockfd[count], recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from[count], &fromlen)) < 0) {
                        if (errno == EINTR) {
                            continue;
                        }
                        perror("recvfrom error");
                    } 
                    else 
                    {
                        gettimeofday(&(tvrecv[count]), NULL);
                        if (unpack(recvpacket, n,count,flag) == -1) //如果收到的包是上次的，则重新接收包。
                        {
                            //printf("hello1\n");//debug 输出了多个hello1
                            continue;
                        } 
                        else //如果是这次的，就跳出循环 
                        {
                            //printf("线程%d收到响应包\n",count);
                            received = 1;
                            break;
                        }
                    }
                }
            }
        }
        //printf("received=%d\n",received);//debug  received都为0
        if(received==1)
        {
            if(flag==1)
            {
                nreceived[count]++;
            }
            else
            {
                online[count]=1;
            }
        }
        else
        {
            if(flag==1)
            {
                lq[count].onedelay=-1;//表示无法收到包
                lq[count].multidelay[cir_i[count]]=-1;
                cir_i[count]++;
                // printf("ok\n");
            }
            else
            {
                online[count]=0;
            }

        } 
}

void cal_standard_deviation(int count)
{
    float sum=0;
    float average=0;
    int i;
    int n=N;//参与计算的总样本
    for(i=0;i<=N-1;i++)
    {
        if(lq[count].multidelay[i]==-1) 
        {
            n--;
            continue;
        }
        sum+=lq[count].multidelay[i];
    }
    if(n==0) 
    {
        lq[count].averagedelay=-1;
        lq[count].standard_deviation=-999;
        return;
    }
    average=sum/n;
    lq[count].averagedelay=average;
    float mean_sum=0;
    for(i=0;i<=N-1;i++)
    {
        if(lq[count].multidelay[i]==-1) continue;
        mean_sum+=(average-lq[count].multidelay[i])*(average-lq[count].multidelay[i]);
    }
    lq[count].standard_deviation=sqrt(mean_sum/(n-1));

}

void cal_loss_rate(int count)
{
    int i;
    int loss_num=0;
    for(i=0;i<=N-1;i++)
    {
        if(lq[count].multidelay[i]==-1)
        {
            loss_num++;
        }
    }
    lq[count].received=N-loss_num;
    lq[count].loss_rate=(float)loss_num/N;
}

void statistics(int count,int flag)
{       
    if(flag==1)
    {
        printf("\n--------------------5G设备%d:10.3.%d\.1 LinkQuality statistics-------------------\n",count,count);
        // printf("In past %d packets, %d received , %%%d lost\n",nreceived,(nsend-nreceived)/nsend*100);
        printf("In past %d packets,average delay is %f ,%d received , loss rate is %f,flutuate is %f\n",N,lq[count].averagedelay,lq[count].received,lq[count].loss_rate,lq[count].standard_deviation);
    }
    else
    {
        if(online[count]==1)
        {
            printf("5G设备%d:10.3.%d\.1在线\n",count,count);
        }
        else
        {
            printf("5G设备%d:10.3.%d\.1离线\n",count,count);
        }

    }
    
}

// 线程函数
void* threadFunc(void* arg) {
    int flag = *((int*)arg);  // 获取传递的参数flag
    int count = *((int*)(arg + sizeof(int)));  // 获取传递的参数循环次数
    //printf("flag=%d count=%d\n",flag,count);
    //printf("Thread %d started\n", count);
    // 线程操作
    
    //生成一个类似10.3.count.1的字符串
    char IP[20];
    // 使用 sprintf 函数将 int 类型的变量 count 转换为字符串，并将其与其他字符串拼接
    sprintf(IP, "10.3.%d.1", count);
    const char *dest_ip = IP;//行不行
    //printf("%s\n",dest_ip);

    struct hostent *host;
    struct protoent *protocol;
    if( (protocol=getprotobyname("icmp") )==NULL)
    {       perror("getprotobyname");
            exit(1);
    }
    /*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
    if( (sockfd[count]=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
    {       perror("socket error");
            exit(1);
    }

    // 设置套接字为非阻塞模式
    if (set_nonblocking(sockfd[count]) != 0) {
        close(sockfd[count]);
        exit(1);
    }

    // 设置目标地址信息
    memset(&(dest_addr[count]), 0, sizeof(dest_addr[count]));
    dest_addr[count].sin_family = AF_INET;
    if (inet_pton(AF_INET, dest_ip, &(dest_addr[count].sin_addr)) <= 0) 
    {
        perror("inet_pton");
        close(sockfd[count]);
        exit(1);
    }

    /*获取main的进程id,用于设置ICMP的标志符.注意这里获取的是进程id，可能不能获取到线程的ID*/
    //tid[count]=pthread_self();
    //printf("1tid[count] is %d\n",count);

    if(flag==0)//只看一次发包是否收到
    {
        send_packet(count);
        recv_packet(count,flag);
        //检查是否ping的通，进行赋值
    }
    else//看N次发包是否收到，并且计算出平均时延，丢包率，波动等
    {
		for(int i=0;i<=N-1;i++)
        {
			send_packet(count);
            recv_packet(count,flag);
        }
        cal_standard_deviation(count);
        cal_loss_rate(count);

    }
    statistics(count,flag); /*进行统计*/

    //printf("Thread %d ended\n", count);                      
    close(sockfd[count]);
    pthread_exit(NULL);  // 线程退出
}

int main(int argc, char *argv[])
{
    int flag=0;
    if (argc > 1 && strcmp(argv[1], "-lq") == 0)
     {
        flag=1;
        // 使用结构体类型
        lq = (LQ *)malloc((Node+1) * sizeof(LQ));
    } 
    else
    {
        // 使用int类型
        online = (int *)malloc((Node+1) * sizeof(int));
    }

    pthread_t threads[Node+1];// 创建多个线程

    //向可能的在线用户发送ICMP包
    for(int i=1;i<=Node;i++)
    {
        //创建多线程，将i，flag作为参数放进去,在线程函数中就把值赋给链路质量数组
        int* arg = malloc(2 * sizeof(int));  // 为线程参数分配内存
        memcpy(arg, &(flag), sizeof(int)); // 复制参数flag到线程参数
        memcpy(arg + 1, &i, sizeof(int)); // 复制循环次数到线程参数
        pthread_create(&threads[i], NULL, threadFunc, arg);  // 创建线程并传递参数
    }

    // 等待所有线程完成
    for (int i = 1; i <= Node; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    //释放空间
    if(flag==1)
    {
        free(lq);
    }
    else
    {
        free(online);
    }
    return 0;
}