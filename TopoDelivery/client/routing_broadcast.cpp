
#include <iostream>
#include <chrono>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <stack>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <mutex>
#include <json/json.h>
#include <curl/curl.h>
#include <fstream>

extern "C" {
#include <string.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <linux/route.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <error.h>
}

#include "multicast.h"
#include "http_interfaces.h"
#include "client.h"

using namespace std;



// meshvxlan1地址
string mesh_addr[routing_num] = {
	"0","0","0","0","0","0","10.2.6.1",
	"0",
	"0","0","0","0","10.2.12.1","0","0","0","0","0","0","0","0","0","0","0","0",
	"0",
	"0","10.2.27.1","0","0","0","0"
};

// 5Gvxlan0地址
string fiveG_addr[routing_num] = {
	"0","0","0","0","10.1.4.1","0","10.1.6.1",
	"0",
	"0","0","0","0","10.1.12.1","0","0","0","0","0","0","0","0","0","0","0","0",
	"0",
	"0","0","0","0","0","0"
};


// 大网地址网桥
string bignet_addr[routing_num] = {
	"0","0","0","0","10.3.4.0","0","10.3.6.0",
	"0",
	"0","0","0","0","10.3.12.0","0","0","0","0","0","0","0","0","0","0","0","0",
	"0",
	"0","10.3.27.0","0","0","0","0"
};

// 路由表类
class routingTable
{
public:
	// 目的地址
	string R_dest_addr = "";
	// 下一跳地址
	string R_next_addr = "";
	// 跳数
	short R_dist = 0;
	// 下一跳接口地址
	string R_iface_addr = "";
	// 下一跳接口类型
	string R_next_type = "";
	//总距离
	int dist = 0;
	// 下挂PCIP
	//string R_PC_addr = "";
	short R_fuse_id = -1;
	routingTable();
	~routingTable();
private:

};
// 构造函数
routingTable::routingTable()
{
}
// 析构函数
routingTable::~routingTable()
{
}
// 创建互斥锁
std::mutex mtx; 
// 终端用户输入的数据：<ID_Number> <5G_server_IP> <5G_server_PORT> <mesh_broadcast_IP> <mesh_IP>
// 其中，将核心的地址信息存入全局变量中，便于接收函数使用。
char ID[3];//设备ID
char fiveG_ip[20]; 
char mesh_broadcast_IP[20]; 
char mesh_IP[20]; 
//定义mesh路径权重
extern int mesh_signal_strength[routing_num][strength_num];
//定义5G路径权重
extern int fiveG_signal_strength[routing_num][strength_num];
//未压缩的拓扑距离
int mesh_topu_source[routing_num][routing_num];
int fiveG_topu_source[routing_num][routing_num];
//标志是否收到消息，以进行路由计算
bool received=0;
std::string interfaceName_5G;
std::string interfaceName_Mesh;
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
//获取电台数据函数
void get_mesh_data(int (&mesh_signal_strength)[routing_num][strength_num], int id)
{
    char addr[40];
    //声明一个CURL句柄
    CURL* curl;
    //声明一个CURLcode类型的变量，存储CURL操作的返回代码
    CURLcode res;
    string response;
    sprintf(addr,"http://192.168.%d.9/query",id);
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
    for (const Json::Value& info : infoArray) 
	{
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
    cout << "提取的原始数据："<< endl;
	for (const auto& data : extractedData) 
	{
		std::cout << "nodeid: " << data.first << std::endl;
		std::cout << "Data: ";
		int row = data.first;
		int j = 0;
		for (const auto& value : data.second) 
		{
			mesh_signal_strength[row][j++] = value;
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}
    // cout << "保存到拓扑表的数据："<<endl;
    // for(int i = 0; i < routing_num; i++)
    // {
    //     for(int j = 0; j < strength_num; j++)
    //     {
    //         cout << mesh_signal_strength[i][j] << " ";
    //     }
    //     cout << endl;
    // }
}
//10机制转为2进制函数，除以2取余
string ten_to_two(int num)
{
	stack<int> binaryStack;
	string binary = "";
	// 特殊情况处理
	if (num == 0)
		binary = "00000000";
	else
	{
		while (num > 0)
		{
			//取余
			int remainder = num % 2;
			//压栈，先进后出
			binaryStack.push(remainder);
			//除以2
			num /= 2;
		}
		// 判断栈队列是否为空
		while (!binaryStack.empty())
		{
			// 获取栈顶数据并转换为string类型并加入binary字符串
			//高位在左，低位在右
			binary += to_string(binaryStack.top());
			//出栈
			binaryStack.pop();
		}
	}
	//高位为0时，补零
	while(binary.size() < 8)
	{
		binary.insert(0, 1, '0');
	}
	return binary;
}
// 注意SDK返回数据是字符型，所有数据都需要转换为10进制数据后使用
// 得到一个节点与其余所有节点的连接强度
void get_one_point_strength(int str[strength_num], int * signalStrength)
{
	for (int n = 0; n < strength_num; n++)
	{
		int num;
		// // 转换为10进制
		// num = std::stoi(str[n]);
		num = str[n];
		//打印转换后的数据是否有误
		//cout << "str:" << str[n] << "\t\t" << "num:" << num;
		//转换为2进制
		string binary = ten_to_two(num);
		//cout << "\t\t" << "binary: " << binary << endl;
		//获取本节点与其他节点的连接强度
		
		for (int i = binary.size() - 1; i >= 0; i -= 2)
		{
			// 两位为一组,高位在左，低位在右
			*(signalStrength++) = ((int)binary[i] - 48) * 1 + ((int)binary[i - 1] - 48) * 2;
		}
	}

}

// 建立拓扑图
// str是字符串数组，就是D1-D8
// 第二个参数是引用
// flag：>0:5G 0:自组网
void get_topu(int str[][strength_num], int(&topu)[routing_num][routing_num], int flag)
{
	//数值等于0表示无连接，1表示连接信号为3,2表示连接信号为2,3表示连接信号为1
	//并且连接边的权值为1，最多一个子网33个节点，D1-D8,4*8+1=33。
	for (int i = 0; i < routing_num; i++)
	{
		int signalStrength[routing_num];
		//获取信号连接强度
		get_one_point_strength(str[i],signalStrength);
		//构建拓扑图，将与自己的连接设置为0
		for (int j = 0; j < routing_num; j++)
		{
			// 将自己与自己的连接设置为0
			if (j == i)
			{
				topu[i][j] = 0;
			}
			// 填入先前获得的信号强度
			else
			{
				//mesh
				if(flag == 0)
				{
					if (signalStrength[j] == 3)
						topu[i][j] = 1;
					else if (signalStrength[j] == 2)
						topu[i][j] = 2;
					else if (signalStrength[j] == 1)
						topu[i][j] = 3;
					else
						topu[i][j] = 0;
				}
				//5G
				else
				{
					if (signalStrength[j] > 0)
					{
						topu[i][j] = 1;
					}
					else
					{
						topu[i][j] = 0;
					}
				}
			}

		}
	}
}


// 将SDK接口数据转换为拓扑连接图
// 两个节点间的信号强度连接都为3判定为直接连接，其余为非直接连接。

// 在所有未访问的节点中找到距离值最小的节点作为“当前节点”
int minDistance(int dist[], bool sptSet[])
{

	// 初始化min值
	int min = INT_MAX, min_index;

	for (int v = 0; v < V; v++)
		if (sptSet[v] == false && dist[v] <= min)//这里是小于等于，若有两个相同的最小值时，则是返回索引后面找的那个
			min = dist[v], min_index = v;

	return min_index;
}

// 打印构造的距离数组的实用函数
void printSolution(int dist[])
{
	cout << "Vertex \t Distance from Source" << endl;
	for (int i = 0; i < V; i++)
		cout << i << " \t\t\t\t" << dist[i] << endl;
}

// 为使用邻接矩阵表示的图实现
// Dijkstra的单源最短路径算法的函数
// dist[v]是源节点到其余每个节点的距离
// sptSet[u]是保存已处理的节点的
// u是当前处理节点的索引
// v是遍历其余节点的索引
void dijkstra(int fiveG_topu_source[V][V], int graph[V][V], int src, routingTable* (&routing_table)[routing_num])
{
	// 输出数组。Dist [i]最短
	// SRC到I的距离
	int dist[V];

	// 如果顶点i包含在最短路径中，则sptSet[i]的值为true
	// 最终确定SRC到I的路径树或最短距离
	bool sptSet[V];

	// 将所有距离初始化为INFINITE，将stpSet[]初始化为false
	for (int i = 0; i < V; i++)
		dist[i] = INT_MAX, sptSet[i] = false;

	// 源点到自身的距离总是0
	dist[src] = 0;

	// 初始化源节点的跳数为0，方便后续更新跳数，后面更新路由表不需要更新源节点的表项
	routing_table[src]->R_dist = 0;
	routing_table[src]->R_next_addr = bignet_addr[src];
	

	// 找出所有顶点的最短路径
	for (int count = 0; count < V - 1; count++) {
		// 从尚未处理的顶点集合中选择距离最小的顶点。在第一次迭代中，U总是等于SRC。
		//最小距离(dist, sptSet);
		int u = minDistance(dist, sptSet);

		// 将选中的顶点标记为已处理
		sptSet[u] = true;

		// 更新选中顶点的相邻顶点的dist值。
		for (int v = 0; v < V; v++)
		{
			int distuv = 0;
			int five_mesh_flag = 0;
			//判断使用5G还是mesh
			if(fiveG_topu_source[u][v] == 0)
			{
				distuv = graph[u][v];
				//选择mesh
				five_mesh_flag = 0;
				//cout << "mesh" << endl;
			}
			else
			{
				distuv = fiveG_topu_source[u][v];
				//选择5G
				five_mesh_flag = 1;
				//cout << "5G" << endl;
			}
			// 仅当dist[v]不存在于sptSet中，
			//且存在从u到v的边，且src经过u到v的路径总权重
			//小于dist[v]的当前值时更新dist[v]
			if ((!sptSet[v] && graph[u][v]) || (!sptSet[v] && fiveG_topu_source[u][v]))
			{
				if (dist[u] != INT_MAX && dist[u] + distuv < dist[v])
				{
					// 若源节点经过u节点中继到v节点的距离小于源节点直接到v节点的距离
					// 更新u到v的边的权重
					dist[v] = dist[u] + distuv;
					routing_table[v]->dist = dist[v];
					// 保存路径
					// 检索是否有到当前v节点的路由，有则更新路由表，否则新增表项
					int k = 0;
					for (k = 0; k < routing_num; k++)
					{
						if (routing_table[k]->R_dest_addr == bignet_addr[v])
							break;
					}
					// 遍历完没有相同的
					if (k == routing_num)
					{
						//新增到节点v的路由
						// 填写目的地址为当前v节点
						routing_table[v]->R_dest_addr = bignet_addr[v];
						// 填写跳数为u节点的跳数加1
						routing_table[v]->R_dist = routing_table[u]->R_dist + 1;
						// 当前计算节点为源节点，与源节点直接相连，下一跳就是源节点自己
						if (routing_table[u]->R_dist == 0)
						{
							// mesh
							if (five_mesh_flag == 0)
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = mesh_addr[v];
								routing_table[v]->R_next_type = "vxlan1";
                                // 填写下一跳接口地址
                                routing_table[v]->R_iface_addr = mesh_addr[src];
							}
							//5G
							else
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = fiveG_addr[v];
								routing_table[v]->R_next_type = "vxlan0";
                                // 填写下一跳接口地址
                                routing_table[v]->R_iface_addr = fiveG_addr[src];
							}
						}
						else
						{
							routing_table[v]->R_next_type = routing_table[u]->R_next_type;
                            routing_table[v]->R_next_addr = routing_table[u]->R_next_addr;
                            routing_table[v]->R_iface_addr = routing_table[u]->R_iface_addr;
						}
					}
					// 否则更改已有的条目
					else
					{
						// 填写跳数为u节点的跳数加1
						routing_table[v]->R_dist = routing_table[u]->R_dist + 1;
						if (routing_table[u]->R_dist == 0)
						{
							// mesh
							if (five_mesh_flag == 0)
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = mesh_addr[v];
								routing_table[v]->R_next_type = "vxlan1";
                                // 填写下一跳接口地址
                                routing_table[v]->R_iface_addr = mesh_addr[src];
							}
							//5G
							else
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = fiveG_addr[v];
								routing_table[v]->R_next_type = "vxlan0";
                                // 填写下一跳接口地址
                                routing_table[v]->R_iface_addr = fiveG_addr[src];
							}
						}
						else
						{
							// //填写下一跳地址
							routing_table[v]->R_next_type = routing_table[u]->R_next_type;
                            routing_table[v]->R_next_addr = routing_table[u]->R_next_addr;
                            routing_table[v]->R_iface_addr = routing_table[u]->R_iface_addr;
						}
					}
					//记录当前路由节点
					routing_table[v]->R_fuse_id = v;
				}

			}
		}
	}
	// 打印构造的distance数组
	//printSolution(dist);
}

// 备份内核路由表
void backups_routing_table()
{
	// 调用route命令，并将输出保存到文件route_output.txt
	string command = "route > route_output.txt";
	// 系统调用
	int result = system(command.c_str());
	// 检查系统调用是否成功
	if (result == 0)
	{
		cout << "路由命令执行成功" << endl;
	}
	else
	{
		cout << "路由命令更新失败" << endl;
	}

}

std::string exec(const char* cmd) {
    std::string result = "";
    char buffer[128];
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

//添加路由表
void add_route(routingTable* routing_table_item, int src)
{
	// std::string scriptPath = "./add_route.sh";
	std::string sudoPermission = "sudo";
    std::string destination = routing_table_item->R_dest_addr;
    std::string next_addr = routing_table_item->R_next_addr;
    std::string interface = routing_table_item->R_next_type;
	std::string command  = "";
	std::string command2 = "";
    command = sudoPermission + " "  +  "ip route add"+ " " + destination + "/24 " + "via " + " " + next_addr + " " + "dev" + " " + interface;
	cout << command << endl;
	// 使用system函数执行shell命令
	int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "脚本执行成功" << std::endl;
    } else {
        std::cout << "脚本执行失败" << std::endl;
		perror("system");
    }
	//以下两条路由只需要添加一次
	if(mesh_addr[src] == "0" || fiveG_addr[src] == "0")
	{
		//如果当前源节点不是融合节点
		if(mesh_addr[src] == "0" && mesh_addr[routing_table_item->R_fuse_id] != "0")
		{
			command2 = sudoPermission + " "  +  "ip route add"+ " " + mesh_addr[routing_table_item->R_fuse_id] + " via" + " " + next_addr;
		}
		//如果当前源节点不是融合节点
		if(fiveG_addr[src] == "0" && fiveG_addr[routing_table_item->R_fuse_id] != "0")
		{
			command2 = sudoPermission + " "  +  "ip route add" + " " + fiveG_addr[routing_table_item->R_fuse_id] + " via" + " " + next_addr;
		}
		cout << command2 << endl;
		// 使用system函数执行shell命令
		result = std::system(command2.c_str());
		if (result == 0) {
			std::cout << "脚本执行成功" << std::endl;
		} else {
			std::cout << "脚本执行失败" << std::endl;
			perror("system");
		}
	}
}

// shell查询路由表项
int bash_check_route(routingTable* routing_table_item)
{
	std::string scriptPath = "./check_route.sh";
	std::string sudoPermission = "sudo";
    std::string destination = routing_table_item->R_dest_addr;
    std::string gateway = routing_table_item->R_next_addr;
    std::string interface = routing_table_item->R_next_type;

	std::string command = sudoPermission + " " + scriptPath + " " + destination + " "  + gateway + " " + interface;

	string scriptOutput = exec(command.c_str());

	//路由标目存在
	if(scriptOutput == "1")
	{
		return 1;
	}
	//路由条目不存在
	else
	{
		return 0;
	}
}
// shell删除路由表项
void delete_route(routingTable* routing_table_item, int src)
{
	// std::string scriptPath = "./delete_route.sh";
	std::string sudoPermission = "sudo";
    std::string destination = routing_table_item->R_dest_addr;
    std::string gateway = routing_table_item->R_next_addr;
    std::string interface = routing_table_item->R_next_type;
	std::string command = "";
	std::string command2 = "";
	command = sudoPermission + " "  +  "ip route del"+ " " + destination + "/24 " + "via " + " " + routing_table_item->R_next_addr + " " + "dev" + " " + interface;
	cout << command << endl;
	// 使用system函数执行shell命令
    int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "脚本执行成功" << std::endl;
    } else {
        std::cout << "脚本执行失败" << std::endl;
		perror("system");
    }
	//以下两条路由只需要添加一次
	if(mesh_addr[src] == "0" || fiveG_addr[src] == "0")
	{
		//如果当前源节点不是融合节点
		if(mesh_addr[src] == "0" && mesh_addr[routing_table_item->R_fuse_id] != "0")
		{
			command2 = sudoPermission + " "  +  "ip route del"+ " " + mesh_addr[routing_table_item->R_fuse_id] + " via" + " " + routing_table_item->R_next_addr;
		}
		//如果当前源节点不是融合节点
		if(fiveG_addr[src] == "0" && fiveG_addr[routing_table_item->R_fuse_id] != "0")
		{
			command2 = sudoPermission + " "  +  "ip route del"+ " " + fiveG_addr[routing_table_item->R_fuse_id] + " via" + " " + routing_table_item->R_next_addr;
		}
		cout << command2 << endl;
		// 使用system函数执行shell命令
		result = std::system(command2.c_str());
		if (result == 0) {
			std::cout << "脚本执行成功" << std::endl;
		} else {
			std::cout << "脚本执行失败" << std::endl;
			perror("system");
		}
	}

}

// 更新内核路由表
void update_routing_table(int src, routingTable* routing_table[routing_num], routingTable* previous_routing_table[routing_num])
{
	// 获取当前路由表
	backups_routing_table();
	//更新路由表，因为第四台电脑没有，所以要减1
	for (int i = 0; i < routing_num; i++)
	{
		if (i == src)
		{
			continue;
		}
		if(routing_table[i]->R_dest_addr != "")
		{
			//查询上一次路由表中是否存在当前处理的表项
			int j = 0;
			for(j = 0; j < routing_num; j++)
			{
				if(routing_table[i]->R_dest_addr == previous_routing_table[j]->R_dest_addr)
				{
					break;
				}
			}
			//若存在，对比是否需要更新
			if(j != routing_num)
			{
				//链路发生变化,不能是链路链路变好才更新，因为有可能原来好的链路确实不存在了
				if(routing_table[i]->R_next_type != previous_routing_table[j]->R_next_type
					|| routing_table[i]->R_dist != previous_routing_table[j]->R_dist
					|| routing_table[i]->dist != previous_routing_table[j]->dist)
				{
					//删除路由
					delete_route(previous_routing_table[j], src);
					//添加新路由
					add_route(routing_table[i], src);
				}
				else
				{
					cout << "无需更新" << endl;
				}
			}
			//不存在则直接添加
			else
			{
				add_route(routing_table[i], src);
			}
		}

	}
}

bool isInterfaceUp(const std::string& interfaceName) 
{
    std::string path = "/sys/class/net/" + interfaceName + "/operstate";
    std::ifstream file(path);
    std::string state;
    if (file >> state) {
        return state == "up";
    }
    return false;
}

void* routing_task(void *arg)
{
	//传入参数为设备号
	int src_num = std::atoi((char *)arg);
    struct timespec sleepTime;
    sleepTime.tv_sec = ROUTING_TIME; //休眠3秒
    sleepTime.tv_nsec = 0; 
    //定义路由表项结构体
    routingTable* current_routing_table[routing_num];
    routingTable* previous_routing_table[routing_num];

    // 初始化路由表类数组
    for (int i = 0; i < routing_num; i++)
    {
        current_routing_table[i] = new routingTable();
        previous_routing_table[i] = new routingTable();
    }
    /*以下工作需要重复执行， 注释部分是调试信息输出*/
    while(1)
    {
		if(received==0)
		{
			sleep(1);
			continue;
		}
		//加锁
		mtx.lock();
		//若是自己有电台连接，则自己获取电台的数据
		if(isInterfaceUp(interfaceName_Mesh))
		{
			get_mesh_data(mesh_signal_strength, src_num);
		}
        //获取mesh拓扑图
        get_topu(mesh_signal_strength, mesh_topu_source, 0);
        //cout << "5G数据处理" << endl;
        //获取5G拓扑图
        get_topu(fiveG_signal_strength, fiveG_topu_source, 1);
		for(int i = 0; i < 32; i++)
		{
			for(int j = 0; j < 32; j++)
			{
				cout << fiveG_topu_source[i][j] << " ";
			}
			cout << endl;
		}
        //运行Dijkstr算法,计算路由表
        dijkstra(fiveG_topu_source, mesh_topu_source, src_num, current_routing_table);
		// 打印路由表
		cout << "R_dest_addr\tR_next_addr\tR_dist\tR_iface_addr\tR_next_type\tR_fuse_id" << endl;
		for (int i = 0; i < routing_num; i++)
		{
			if (i == src_num)
				continue;
			if(current_routing_table[i]->R_dest_addr != "")
			{
				cout << current_routing_table[i]->R_dest_addr << "\t" << current_routing_table[i]->R_next_addr << "\t"
				<< current_routing_table[i]->R_dist << "\t" << current_routing_table[i]->R_iface_addr << "\t" << current_routing_table[i]->R_next_type
				<< "\t\t" << current_routing_table[i]->R_fuse_id<< endl;
			}

		}
        //更新内核路由表
        update_routing_table(src_num, current_routing_table, previous_routing_table);
        //更新先前路由
        for (int i = 0; i < routing_num; i++)
        {
            previous_routing_table[i]->R_dest_addr = current_routing_table[i]->R_dest_addr;
            previous_routing_table[i]->R_next_addr = current_routing_table[i]->R_next_addr;
            previous_routing_table[i]->R_next_type = current_routing_table[i]->R_next_type;
            previous_routing_table[i]->R_iface_addr = current_routing_table[i]->R_iface_addr;
            previous_routing_table[i]->dist = current_routing_table[i]->dist;
            previous_routing_table[i]->R_dist = current_routing_table[i]->R_dist;
            previous_routing_table[i]->R_fuse_id = current_routing_table[i]->R_fuse_id;
        }
		received=0;//使received为0，需要收到消息才能进行路由计算。
		//解锁
		mtx.unlock();
        //休眠3秒
        nanosleep(&sleepTime, nullptr);
    }
	return nullptr;
}

// 任务2：接收拓扑数据任务，每隔1秒执行一次
void* recv_task(void *)
{
    struct timespec sleepTime;
    sleepTime.tv_sec = BROADCAST_TIME;
    sleepTime.tv_nsec = 0;
	cout<<"enter recv_task"<<endl;
    while (1)
    {
		cout<<"enter while condition"<<endl;
		
        /*接收拓扑信息操作*/
		// if ((fiveG_ip[0] != '0')&&(mesh_broadcast_IP[0] != '0')&&(mesh_IP[0] != '0')) 
		if(isInterfaceUp(interfaceName_5G) && isInterfaceUp(interfaceName_Mesh))
		{
			//这是融合节点的情况，所以要从mesh接收5g拓扑，从5g接收mesh拓扑。
			
			//加锁
			mtx.lock();
			cout<<"enter 1th condition"<<endl;
			int num = recv_fiveG_tuopu(mesh_IP, mesh_broadcast_IP);    
			if(num==0) received=1;//成功接收，可以进行路由计算了
			mtx.unlock();

		}
		// else if ((fiveG_ip[0] != '0')&&(mesh_broadcast_IP[0] == '0')&&(mesh_IP[0] == '0'))
		else if(isInterfaceUp(interfaceName_5G) && !isInterfaceUp(interfaceName_Mesh))
		{
			//这是单一只连有5g节点的情况，所以要从基站收到mesh拓扑和5g拓扑
			mtx.lock();
			cout<<"enter 2th condition"<<endl;
			recv_mesh_tuopu(fiveG_ip);
			fiveG_recv_fiveG_tuopu(fiveG_ip);
			received=1;//成功接收，可以进行路由计算了。此处不严谨，因为recv_mesh_tuopu、fiveG_recv_fiveG_tuopu函数不可修改且没有返回值，不知道是否成功接收
			mtx.unlock();

		}
		// else if ((fiveG_ip[0] == '0')&&(mesh_broadcast_IP[0] != '0')&&(mesh_IP[0] != '0'))
		else if(!isInterfaceUp(interfaceName_5G) && isInterfaceUp(interfaceName_Mesh))
		{
			//这是单一只连有自组网节点的情况，所以要从电台收到mesh拓扑和5g拓扑，假设mesh拓扑可以从电台提供的接口获得，所以只接收了5g拓扑
			mtx.lock();
			cout<<"enter 3th condition"<<endl;
			int num = recv_fiveG_tuopu(mesh_IP, mesh_broadcast_IP);
			if(num==0) received=1;//成功接收，可以进行路由计算了
			mtx.unlock();
		}
        nanosleep(&sleepTime, nullptr);
    }
    return nullptr;
    
}

void* httpserver(void *)
{
    //定义http接口实例类
	HTTPServerManager httpServerManager;
	httpServerManager.SetHttpsever(true);
    return nullptr;
    
}



/*Usage: ./code <ID_Number> <5G_server_IP> <mesh_broadcast_IP> <mesh_IP>*/
/*以上数据若是没有直接输入0， 不能空着*/
int main(int argc, char *argv[])
{
	//将终端用户输入的ip信息存入全局变量中
	strcpy(ID,argv[1]);
	strcpy(fiveG_ip, argv[2]);
	strcpy(mesh_broadcast_IP, argv[3]);
	strcpy(mesh_IP, argv[4]);
    // 创建线程变量
    pthread_t t1;
    pthread_t t2;
	pthread_t t3;


    //判断参数
    if(argc != 5)
    {
        cout << "Usage: ./" << argv[0] << "<ID_Number> <5G_server_IP> <mesh_broadcast_IP> <mesh_IP>" << endl;
        return -1;
    }

	//用于检查某些接口是否中途断开
	interfaceName_5G = "pcilan3"; // interface name
	if(strcmp(ID,"12")!=0)//when the number of ID is "7",the interface have some changes because some problems.
	{
		interfaceName_Mesh = "pcilan2"; // interface name
	}
	else
	{
		interfaceName_Mesh = "pcilan1"; // interface name
	}


    // 创建线程并启动任务
    //拓扑接收任务
    pthread_create(&t1, nullptr, recv_task, nullptr);
    //路由计算任务
    pthread_create(&t2, nullptr, routing_task, (void *)argv[1]);
	//接口服务器
	pthread_create(&t3, nullptr, httpserver, nullptr);
    // 主线程等待线程结束
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
	pthread_join(t3, nullptr);
    return 0;
}
/*把该程序以守护进程运行在后台，开机跑完脚本后自启动*/