//本文件目的：拓扑传递的服务端，用于获取5G拓扑并且压缩，获取自组网拓扑，并且发送给5G、自组网设备
//修改时间：20231206
//5G探测目的：打印出5G设备是否在线的情况，如果再多输入参数-lq，则还会打印5G的链路质量。（结果会存在数组中）
//compile_code:g++ multicast.cpp server20231206.cpp -o server -pthread -lm  -I/usr/include/jsoncpp -lcurl -ljsoncpp
#include <curl/curl.h>
#include <iostream>
#include <json/json.h>///usr/include/jsoncpp/
#include <vector>
#include <utility>
#include <bitset>
#include <sstream>
#include "multicast.h"

extern "C" {
#include <string.h>
#include <linux/route.h>
#include <sys/ioctl.h>
#include <error.h>
#include <math.h>
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
}


using namespace std;

#define Node 16
#define N 5
#define PACKET_SIZE 4096
#define sleep_time 5
#define routing_num 32
#define strength_num 8 
#define Max_Wait_Times 1

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

// //定义mesh路径权重
// int mesh_signal_strength[routing_num][strength_num];

// //要通过组播发送的数组
// int fiveg_signal_strength[Node][Node/4] = {0};

//extern
extern int mesh_signal_strength[routing_num][strength_num];
extern int fiveg_signal_strength[Node][Node/4] = {0};

// 功能 ：将整数转换为二进制字符串
std::string toBinary(int num) {
    // 将整数转换为二进制字符串
    std::bitset<2> binary(num);
    return binary.to_string();
}

// 功能 ：将8位二进制转换成十进制
int concatenateAndConvert(std::string binary1, std::string binary2, std::string binary3, std::string binary4) {
    // 将两个二进制数进行拼接
    std::string concatenatedBinary = binary1 + binary2 + binary3 + binary4;

    // 将拼接后的二进制数转换为十进制整数
    int decimalNumber = std::stoi(concatenatedBinary, nullptr, 2);

    return decimalNumber;
}

// 功能处理源数据
void convertToOneDimensionalArray(const int* online, int fiveg_signal_strength[Node][Node/4] ) {
    //中间数组[16*16]
    int temp[Node][Node] = {0};
    for (int i = 0; i < Node; i++) {
       if(online[i] == 1){
           for (int j = 0; j < Node; j++) {
                if(online[j] == 1){
                    temp[i][j] = 1;
                }else{
                    temp[i][j] = 0;
                }                                        
            }
       }
       else
       {
            for(int j=0;j<Node;j++)
            {
                temp[i][j]=0;
            }
       }
    }
    for(int i=0;i<Node;i++)
    {
        for(int j=0;j<Node;j++)
        {
            if(i==j)
            {
                temp[i][j]=0;//20231211改，自己到自己为0
            }
        }
    }

    cout<<"5G原始拓扑"<<endl;
    for(int i=0;i<Node;i++)
    {
        for(int j=0;j<Node;j++)
        {
            cout<<temp[i][j]<<" ";
        }
        cout<<endl;
    }

    //[16*16]数组转换成[16*4]的发送数组
    if(Node%4!=0)
    {
        cout<<"Bad Number of Node!It must:Node%4==0"<<endl;
    }
    cout<<"5G压缩拓扑:行数"<<endl;
    for(int i=0;i<=Node-1;i++)
    {
        for(int j=0;j<=Node/4-1;j++)
        {
            fiveg_signal_strength[i][j]=(temp[i][4*j+0])+((temp[i][4*j+1])<<2)+((temp[i][4*j+2])<<4)+((temp[i][4*j+3])<<6);
            cout<<fiveg_signal_strength[i][j]<<" ";
        }
        cout<<i<<endl;
    }
    
            
    
}

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
        int n;
        extern int errno;
        int received=0;
        socklen_t fromlen = sizeof(struct sockaddr);
        fromlen=sizeof(from[count]);
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = Max_Wait_Times;// 设置最大等待时间为Max_Wait_Times
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
            printf("5G设备%d:10.1.%d\.1在线\n",count,count);
        }
        else
        {
            printf("5G设备%d:10.1.%d\.1离线\n",count,count);
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
    
    //生成一个类似10.1.count.1的字符串
    char IP[20];
    // 使用 sprintf 函数将 int 类型的变量 count 转换为字符串，并将其与其他字符串拼接
    sprintf(IP, "10.1.%d.1", count);
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

void FiveG_detect(int flag)
{
    pthread_t threads[Node+1];// 创建多个线程
    //向可能的在线用户发送ICMP包
    for(int i=1;i<=Node;i++)
    {
        //创建多线程，将i，flag作为参数放进去,在线程函数中就把值赋给链路质量数组
        //int* arg = malloc(2 * sizeof(int));  // 为线程参数分配内存
        int* arg =new int[2]; 
        memcpy(arg, &(flag), sizeof(int)); // 复制参数flag到线程参数
        memcpy(arg + 1, &i, sizeof(int)); // 复制循环次数到线程参数
        pthread_create(&threads[i], NULL, threadFunc, arg);  // 创建线程并传递参数
    }

    // 等待所有线程完成
    for (int i = 1; i <= Node; i++) 
    {
        pthread_join(threads[i], NULL);
    }
    //printf("online size is %d\n",online.size());
}

//获取自组网拓扑并处理

//回调函数
size_t write_callback(void* contents, size_t size, size_t nmemb, string* response)
{
    //size是每个接收到的数据块的大小，nmemb是接收到的数据块的数量
    size_t totalSize = size * nmemb;
    //contents是接收到的数据的指针
    response->append((char*)contents, totalSize);
    //返回接收到的总数据大小，单位字节
    return totalSize;
}

// void main(int argc, char* argv[]) 
void get_mesh_data(int (&mesh_signal_strength)[routing_num][strength_num], int id)
{ 
    char addr[40];
    //声明一个CURL句柄
    CURL* curl;
    //声明一个CURLcode类型的变量，存储CURL操作的返回代码
    CURLcode res;
    string response;
    string id_str=std::to_string(id);
    const char* id_char_array = id_str.c_str();
    sprintf(addr,"http://192.168.%s.9/query",id_char_array);
    //string id_str=std::to_string(id);
    //sprintf(addr,"http://192.168.%s.9/query","14");
    // 进行全局初始化
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // 初始化CURL句柄
    curl = curl_easy_init();
    //检查句柄是否成功初始化
    if(curl)
    {
        //设置HTTP请求自定义方法为“GET”
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        //设置URL
        curl_easy_setopt(curl, CURLOPT_URL, addr);
        //启用libcurl的自动重定向功能，自动跟随重定向响应
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        //设置默认的协议为HTTPS，如果重定向后的URL使用了不同的协议，libcurl将使用指定的协议进行请求
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        //创建一个 curl_slist 结构体，用于存储 HTTP 请求的头部信息
        struct curl_slist *headers = NULL;
        //将 "Content-Type" 设置为 "application/json"。
        headers = curl_slist_append(headers, "Content-Type: application/json");
        //通过 CURLOPT_HTTPHEADER 选项将头部信息添加到 CURL 句柄中。
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        //设置 HTTP 请求的主体数据。在这里，将一个 JSON 字符串作为请求的主体数据。
        const char *data = "{\"topo\":1}";
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        //设置回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        //设置接受函数的字符串
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        //执行请求
        res = curl_easy_perform(curl);
        //错误判断
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
        }
        //打印提取到的数据
        cout << "Response: " << response <<  endl;
        //清理CURL句柄
        curl_easy_cleanup(curl);
    }
    else
    {
        cout << "CURL句柄初始化失败" << endl;
        return;
    }
    //清理CURL全局库
    curl_global_cleanup();
    //声明Value类型变量root以存储 JSON 数据的不同类型，如对象、数组、字符串、整数等
    Json::Value root;
    //CharReader 是一个用于从字符流中解析 JSON 的类
    Json::CharReaderBuilder reader;
    //通过使用 std::istringstream，将字符串转换为输入流
    std::istringstream iss(response);
    //定义errs字符串变量
    std::string errs;
    // 从iss对象中获取输入刘，从输入流中解析 JSON 数据并填充到 Json::Value 对象中
    Json::parseFromStream(reader, iss, &root, &errs);
    //从 root 对象中获取 JSON 数据的特定字段，并将其存储在名为 infoArray 的常量引用中
    const Json::Value& infoArray = root["topo"]["info"];
    //声明 extractedData 的变量，它是一个向量（std::vector）类型，其中每个元素都是一个包含两个值的 std::pair 对象
    //int：设备ID(nodeld)，vector<int>：连接数据(Dx)
    std::vector<std::pair<int, std::vector<int>>> extractedData;
    // 遍历 infoArray 数组
    for (const Json::Value& info : infoArray) {
        //获取当前元素中名为 "nodeId" 的字段的整数值
        int nodeId = info["nodeid"].asInt();
        //获取当前元素中名为 "node" 的字段，它应该是一个包含节点数据的数组。将其存储在 nodeData 常量引用中
        const Json::Value& nodeData = info["node"];
        //定义一个向量变量
        std::vector<int> dataList;
        // 遍历节点数据数组
        //info 是当前数组元素的引用
        for (const Json::Value& value : nodeData) {
            if (value.isInt()) {
                // 提取整数值并将其添加到 dataList 向量中
                dataList.push_back(value.asInt());
            }
        }
        // 将节点 ID 和数据列表添加到提取的数据向量中
        extractedData.push_back(std::make_pair(nodeId, dataList));
    }
    //打印提取的数据
    // int mesh_data[32][8];
    for(int i = 0; i < routing_num; i++)
    {
        for(int j = 0; j < strength_num; j++)
        {
            mesh_signal_strength[i][j] = 0;
        }
    }
    cout << "提取的原始数据："<< endl;
        for (const auto& data : extractedData) {
        std::cout << "nodeid: " << data.first << std::endl;
        std::cout << "Data: ";
        int row = data.first;
         int j = 0;
        for (const auto& value : data.second) {
            mesh_signal_strength[row][j++] = value;
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
    cout << "保存到拓扑表的数据："<<endl;
    for(int i = 0; i < routing_num; i++)
    {
        for(int j = 0; j < strength_num; j++)
        {
            cout << mesh_signal_strength[i][j] << " ";
        }
        cout << endl;
    }
    return;
}

//获取电台数据函数
// void get_mesh_data(int (&mesh_signal_strength)[routing_num][strength_num], int id)
// {
//     char addr[40];
//     //声明一个CURL句柄
//     CURL* curl;
//     //声明一个CURLcode类型的变量，存储CURL操作的返回代码
//     CURLcode res;
//     string response;
//     // if (2 < argc)//怀疑这儿有问题
//     // {
//     //     cout << "usage: get_topu <device id>" << endl;
//     //     return 0;
//     // }
//     sprintf(addr,"http://192.168.%d.9/query",id);
//     // 进行全局初始化
//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     // 初始化CURL句柄
//     curl = curl_easy_init();
//     //检查句柄是否成功初始化
//     if(curl)
//     {
//         //设置HTTP请求自定义方法为“GET”
//         curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
//         //设置URL
//         curl_easy_setopt(curl, CURLOPT_URL, addr);
//         //启用libcurl的自动重定向功能，自动跟随重定向响应
//         curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
//         //设置默认的协议为HTTPS，如果重定向后的URL使用了不同的协议，libcurl将使用指定的协议进行请求
//         curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
//         //创建一个 curl_slist 结构体，用于存储 HTTP 请求的头部信息
//         struct curl_slist *headers = NULL;
//         //将 "Content-Type" 设置为 "application/json"。
//         headers = curl_slist_append(headers, "Content-Type: application/json");
//         //通过 CURLOPT_HTTPHEADER 选项将头部信息添加到 CURL 句柄中。
//         curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//         //设置 HTTP 请求的主体数据。在这里，将一个 JSON 字符串作为请求的主体数据。
//         const char *data = "{\"topo\":1}";
//         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
//         //设置回调函数
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
//         //设置接受函数的字符串
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//         //执行请求
//         res = curl_easy_perform(curl);
//         //错误判断
//         if(res != CURLE_OK)
//         {
//             fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
//         }
//         //打印提取到的数据
//         cout << "Response: " << response <<  endl;
//         //清理CURL句柄
//         curl_easy_cleanup(curl);
//     }
//     else
//     {
//         cout << "CURL句柄初始化失败" << endl;
//     }
//     //清理CURL全局库
//     curl_global_cleanup();
//     //声明Value类型变量root以存储 JSON 数据的不同类型，如对象、数组、字符串、整数等
//     Json::Value root;
//     //CharReader 是一个用于从字符流中解析 JSON 的类
//     Json::CharReaderBuilder reader;
//     //通过使用 std::istringstream，将字符串转换为输入流
//     std::istringstream iss(response);
//     //定义errs字符串变量
//     std::string errs;
//     // 从iss对象中获取输入刘，从输入流中解析 JSON 数据并填充到 Json::Value 对象中
//     Json::parseFromStream(reader, iss, &root, &errs);
//     //从 root 对象中获取 JSON 数据的特定字段，并将其存储在名为 infoArray 的常量引用中
//     const Json::Value& infoArray = root["topo"]["info"];
//     //声明 extractedData 的变量，它是一个向量（std::vector）类型，其中每个元素都是一个包含两个值的 std::pair 对象
//     //int：设备ID(nodeld)，vector<int>：连接数据(Dx)
//     std::vector<std::pair<int, std::vector<int>>> extractedData;
//     // 遍历 infoArray 数组
//     for (const Json::Value& info : infoArray) 
// 	{
//         //获取当前元素中名为 "nodeId" 的字段的整数值
//         int nodeId = info["nodeId"].asInt();
//         //获取当前元素中名为 "node" 的字段，它应该是一个包含节点数据的数组。将其存储在 nodeData 常量引用中
//         const Json::Value& nodeData = info["node"];
//         //定义一个向量变量
//         std::vector<int> dataList;
//         // 遍历节点数据数组
//         //info 是当前数组元素的引用
//         for (const Json::Value& value : nodeData) {
//             if (value.isInt()) {
//                 // 提取整数值并将其添加到 dataList 向量中
//                 dataList.push_back(value.asInt());
//             }
//         }
//         // 将节点 ID 和数据列表添加到提取的数据向量中
//         extractedData.push_back(std::make_pair(nodeId, dataList));
//     }
//     //打印提取的数据
//     cout << "提取的原始数据："<< endl;
// 	for (const auto& data : extractedData) 
// 	{
// 		std::cout << "nodeId: " << data.first << std::endl;
// 		std::cout << "Data: ";
// 		int row = data.first;
// 		int j = 0;
// 		for (const auto& value : data.second) 
// 		{
// 			mesh_signal_strength[row][j++] = value;
// 			std::cout << value << " ";
// 		}
// 		std::cout << std::endl;
// 	}
//     cout << "保存到拓扑表的数据："<<endl;
//     for(int i = 0; i < routing_num; i++)
//     {
//         for(int j = 0; j < routing_num; j++)
//         {
//             cout << mesh_signal_strength[i][j] << " ";
//         }
//         cout << endl;
//     }
// }
// //结束自组网拓扑和处理

void Process()
{

    //online数组 -->  组播发送的数组
    convertToOneDimensionalArray(online, fiveg_signal_strength);

    // //打印转换后的二维数组
    // for (int i = 0; i < Node; i++) {
    //     for (int j = 0; j < Node/4; j++) {
    //         std::cout << fiveg_signal_strength[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
}


string Get5GIp(int i)
{
    return "10.1."+std::to_string(i)+".1";
}

void Send_Topo()
{

    //mesh电台给mesh电台组播 5g拓扑。
    //args:  mesh_ip此设备的电台ip,
    // multicast_ip 组播组ip已经写好 √
    // fiveG_signal_strength要传输的5g拓扑

    string multicast_ip = "239.0.125.1";
    //string multicast_ip = "239.0.253.0";
    //string mesh_ip="192.168.14.12";
    string mesh_ip="10.2.14.1";
    string fiveG_ip="10.1.14.1";
    send_fiveG_tuopu(mesh_ip, multicast_ip, fiveg_signal_strength);

    //通过5G发送
    for(int i = 0 ; i < Node; i ++){
        if(online[i]==1)
        {
            string fiveGIps=Get5GIp(i);
            cout<<"发送给5G的IP"<<fiveGIps<<endl;
            //这里可以多线程处理，fiveG_ip表示这台设备的5Gip,   fiveGIps表示在线设备要组播的ip们, mesh_signal_strength是要发送的mesh拓扑
            send_mesh_tuopu(fiveG_ip, fiveGIps,mesh_signal_strength);
            //fiveG_ip表示这台设备的5Gip,   fiveGIps表示在线设备要组播的ip们, fiveG_signal_strength是要发送的5g拓扑
            fiveG_send_fiveG_tuopu(fiveG_ip, fiveGIps,fiveg_signal_strength);
        }

    }


}

int main(int argc, char *argv[])
{
    //用于确定5G探测的模式，在server.c中，只会使用默认的探测模式
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
    //flag




    //服务端框架
    while(1)
    {
        FiveG_detect(flag);
        Process();
        get_mesh_data(mesh_signal_strength,14);//此处的14是自组网的id号

        // 定
        Send_Topo();
        sleep(sleep_time);
    }
    //服务端框架_结束


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
