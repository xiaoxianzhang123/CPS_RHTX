﻿/*
* Dijkstra代码修改，应用于网络路由，通过SDK接口获得各个节点之间的连接状态，
* 构建各节点之间的拓扑图，信号强度作为边的权重距离，信号强度越大，权值距
* 离越小，根据Dijkstra路由算法构建本节点到其余节点的最短路径。
* 2023年9月18日
* 何梓豪
* 融合通信项目组
* 路由代码
*/
#include <iostream>
#include <limits.h>
#include <stack>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <json/json.h>
//#include <string>
extern "C" {
#include <string.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <error.h>
}
using namespace std;

// 图中的顶点数,后续根据SDK接口获取的节点数量来确定
#define V 4
#define routing_num 4
#define strength_num 1 //D1


// mesh地址
string mesh_addr[routing_num] = {
	"192.168.7.12",
	"0",
	"192.168.25.12",
	"0"
};

// 5G地址
string fiveG_addr[routing_num] = {
	"181.181.7.12",
	"181.181.8.9",
	"0",
	"0"
};


// 大网地址
string bignet_addr[routing_num] = {
	"10.0.7.11",
	"10.0.8.11",
	"10.0.25.11",
	"0"
};

// //下挂PCIP地址
// string PC_addr[][] = {
	
// };
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
//获取电台数据函数
void get_mesh_data(int &(mesh_signal_strength)[routing_num][strength_num])
{
	//后面改为通过SDK接口获取输入数据
   // 输入的 JSON 字符串，后面改为用SDK接口获取数据
    std::string input = "{\"topo\":{\"subnum\":\"n\",\"nodecount\":\"L\",\"info\":[{\"nodeId\":0,\"node\":[12,33,23,44,55,66,67,68]},{\"nodeId\":1,\"node\":[59,10,11,12,13,14,15,16]},{\"nodeId\":2,\"node\":[7,6,5,4,3,2,1,4]}]},\"Status\": \"OK\"";
    //声明Value类型变量root以存储 JSON 数据的不同类型，如对象、数组、字符串、整数等
    Json::Value root;
    //CharReader 是一个用于从字符流中解析 JSON 的类
    Json::CharReaderBuilder reader;
    //通过使用 std::istringstream，将字符串转换为输入流
    std::istringstream iss(input);
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
        int nodeId = info["nodeId"].asInt();
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
		std::cout << "nodeId: " << data.first << std::endl;
		std::cout << "Data: ";
		int row = data.first;
		int j = 0;
		for (const auto& value : data.second) 
		{
			mesh_signal_strength[row][j++] = value
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}
    cout << "保存到拓扑表的数据："<<endl;
    for(int i = 0; i < routing_num; i++)
    {
        for(int j = 0; j < routing_num; j++)
        {
            cout << mesh_signal_strength[i][j] << " ";
        }
        cout << endl;
    }
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
		cout << "str:" << str[n] << "\t\t" << "num:" << num;
		//转换为2进制
		string binary = ten_to_two(num);
		cout << "\t\t" << "binary: " << binary << endl;
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
void get_topu(int str[][strength_num], int(&topu)[routing_num][routing_num])
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
				if (signalStrength[j] == 3)
					topu[i][j] = 1;
				else if (signalStrength[j] == 2)
					topu[i][j] = 2;
				else if (signalStrength[j] == 1)
					topu[i][j] = 3;
				else
					topu[i][j] = 0;
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
			}
			else if(graph[u][v] == 0)
			{
				distuv = fiveG_topu_source[u][v];
				//选择5G
				five_mesh_flag = 1;
			}
			else
			{
				if ((fiveG_topu_source[u][v] <= graph[u][v] ) )
				{
					distuv = fiveG_topu_source[u][v];
					//选择5G
					five_mesh_flag = 1;
				}
				else
				{
					distuv = graph[u][v];
					//选择mesh
					five_mesh_flag = 0;
				}
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
								routing_table[v]->R_next_addr = bignet_addr[src];
								routing_table[v]->R_next_type = "vxlan1";
							}
							//5G
							else
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = bignet_addr[src];
								routing_table[v]->R_next_type = "vxlan0";
							}
						}
						else
						{
							// mesh
							// if (five_mesh_flag == 0)
							// {
							// 	routing_table[v]->R_next_type = "vxlan1";
							// }
							// //5G
							// else
							// {
							// 	routing_table[v]->R_next_type = "vxlan0";
							// }
							//填写下一跳地址
							if(routing_table[u]->R_dist == 1)
							{
								routing_table[v]->R_next_addr = bignet_addr[u];
							}
							else
							{
								routing_table[v]->R_next_addr = routing_table[u]->R_next_addr;
							}
							routing_table[v]->R_next_type = routing_table[u]->R_next_type;
						}
						// 填写下一跳接口地址
						routing_table[v]->R_iface_addr = bignet_addr[src];
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
								routing_table[v]->R_next_addr = bignet_addr[src];
								routing_table[v]->R_next_type = "vxlan1";
							}
							//5G
							else
							{
								// 填写下一跳地址
								routing_table[v]->R_next_addr = bignet_addr[src];
								routing_table[v]->R_next_type = "vxlan0";
							}
						}
						else
						{
							// // mesh
							// if (five_mesh_flag == 0)
							// {
							// 	routing_table[v]->R_next_type = "vxlan1";
							// }
							// //5G
							// else
							// {
							// 	routing_table[v]->R_next_type = "vxlan0";
							// }
							//填写下一跳地址
							if(routing_table[u]->R_dist == 1)
							{
								routing_table[v]->R_next_addr = bignet_addr[u];
							}
							else
							{
								routing_table[v]->R_next_addr = routing_table[u]->R_next_addr;
							}
							routing_table[v]->R_next_type = routing_table[u]->R_next_type;
						}
						// 填写下一跳接口地址
						routing_table[v]->R_iface_addr = bignet_addr[src];
					}

				}

			}
		}
	}

	// 打印构造的distance数组
	printSolution(dist);
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

//l添加路由表
void add_route(routingTable* routing_table_item)
{
	// std::string scriptPath = "./add_route.sh";
	std::string sudoPermission = "sudo";
    std::string destination = routing_table_item->R_dest_addr;
    std::string gateway = routing_table_item->R_next_addr;
    std::string interface = routing_table_item->R_next_type;
	std::string command  = "";

	if(routing_table_item->R_dist == 1)
	{
		command = sudoPermission + " "  +  "ip route add "+ " " + destination + " "  + " dev " + " " + interface;
		cout << command << endl;
	}
	else
	{
		command = sudoPermission + " "  +  "ip route add "+ " " + destination + " " + " via " + " " + gateway + " " + " dev " + " " + interface;
		cout << command << endl;
	}
	// 使用system函数执行shell命令
    int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "脚本执行成功" << std::endl;
    } else {
        std::cout << "脚本执行失败" << std::endl;
		perror("system");
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
void delete_route(routingTable* routing_table_item)
{
	// std::string scriptPath = "./delete_route.sh";
	std::string sudoPermission = "sudo";
    std::string destination = routing_table_item->R_dest_addr;
    std::string gateway = routing_table_item->R_next_addr;
    std::string interface = routing_table_item->R_next_type;
	std::string command = "";
	command = sudoPermission + " " + " ip route del " + " " + destination + " " + " via " + " " +  gateway;

	cout << command << endl;
	// 使用system函数执行shell命令
    int result = std::system(command.c_str());
    if (result == 0) {
        std::cout << "脚本执行成功" << std::endl;
    } else {
        std::cout << "脚本执行失败" << std::endl;
		perror("system");
    }

}

// 更新内核路由表
void update_routing_table(int src, routingTable* routing_table[routing_num])
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
			add_route(routing_table[i]);
		}
		// // 查询表项是否存在
		// int ret =  bash_check_route(routing_table[i]);
		// // 更新路由表项
		// //表项存在
		// if(ret == 1)
		// {
		// 	delete_route(routing_table[i]);
		// 	cout << "没有路由表项，新添表项" << endl;
		// 	bash_add_route(routing_table[i]);
		// }
		// else
		// {
		// 	bash_add_route(routing_table[i]);
		// }
		// 有对应表项已删除或不存在该表项

	}
}

int main(int argc, char* argv[])
{
	 //usage: sendfile [server_port]
   if (2 < argc)
   {
      cout << "usage: Dijkstra_shell_arg [source number]" << endl;
      return 0;
   }
  int src_num = std::atoi(argv[1]);
	//初始化测试mesh路径权重
int mesh_signal_strength[routing_num][strength_num] = {
	{32},
	{0},
	{2},
	{0}
};

//初始化测试5G路径权重
int fiveG_signal_strength[routing_num][strength_num] = {
	{12},
	{3},
	{0},
	{0}
};

//初始化信号拓扑,然后经过获取拓扑函数更改有连接的部分
for(int i = 0; i < routing_num; i++)
{
	for(int j = 0; j < routing_num; j++)
	{
		mesh_signal_strength[i][j] = 0;
		fiveG_signal_strength[i][j] = 0;
	}
}

int mesh_topu_source[routing_num][routing_num];
int fiveG_topu_source[routing_num][routing_num];

// 初始化路由表类数组
routingTable* routing_table_source[routing_num];
for (int i = 0; i < routing_num; i++)
{
	routing_table_source[i] = new routingTable();
}
//调用自组网SDK接口获取本节点与每个节点的连接状态

	cout << "mesh数据处理" << endl;
	//获取mesh拓扑图
	get_topu(mesh_signal_strength, mesh_topu_source);
	cout << "5G数据处理" << endl;
	//获取5G拓扑图
	get_topu(fiveG_signal_strength, fiveG_topu_source);
	cout << "mesh拓扑图" << endl;
	for (int i = 0; i < routing_num; i++)
	{
		for (int j = 0; j < routing_num; j++)
		{
			cout << mesh_topu_source[i][j] << "  ";
		}
		cout << endl;
	}
	cout << "5G拓扑图" << endl;
	for (int i = 0; i < routing_num; i++)
	{
		for (int j = 0; j < routing_num; j++)
		{
			cout << fiveG_topu_source[i][j] << "  ";
		}
		cout << endl;
	}
	//运行Dijkstr算法,计算路由表
	dijkstra(fiveG_topu_source, mesh_topu_source, src_num, routing_table_source);

	// 打印路由表
	cout << "R_dest_addr\t\t\tR_next_addr\t\t\tR_dist\t\t\tR_iface_addr\t\t\tR_next_type" << endl;
	for (int i = 0; i < routing_num; i++)
	{
		if (i == src_num)
			continue;
		if(routing_table_source[i]->R_dest_addr != "")
		{
			cout << routing_table_source[i]->R_dest_addr << "\t\t\t" << routing_table_source[i]->R_next_addr << "\t\t\t"
			<< routing_table_source[i]->R_dist << "\t\t\t" << routing_table_source[i]->R_iface_addr << "\t\t\t" << routing_table_source[i]->R_next_type << endl;
		}

	}
	//更新内核路由表
	//update_routing_table(src_num, routing_table_source);
	return 0;
}