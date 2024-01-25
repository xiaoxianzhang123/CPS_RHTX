//完成版，要安装cpprestSDK库，ethtool工具
#ifdef _WIN64
#include <stdio.h>
#define popen _popen
#define pclose _pclose


#else
#include <cstdio>
#endif
//头文件列表
#include "http_interfaces.h"
#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <chrono>
#include <thread>
#include <array>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "client.h"
#include <string>
#include <locale>

extern std::string localhost;
//TopologyQuery拓扑查询

extern int mesh_topu_source[routing_num][routing_num];
extern int fiveG_topu_source[routing_num][routing_num];


//启动函数
//使用方法：
// 在main函数里：
// HTTPServerManager serverManager;
// 然后
// // 启动 HTTP 服务器
//serverManager.SetHttpsever(true);
//
// 运行一段时间...
//
// 停止 HTTP 服务器
//serverManager.SetHttpsever(false);
// 
//
void HTTPServerManager::SetHttpsever(bool startServer) {
    if (startServer) {
        std::thread httpThread([]() {
            // 启动HTTP服务器监听器的线程
            IDQuery idQueryHandler;
            TopologyQuery topoQueryHandler;
            SourceQuery sourceQueryHandler;

            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            });
        httpThread.detach(); // 释放线程，让它在后台运行
    }
}





//可以在http接口中进行客户端的命令行操作的函数
std::string executeCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    return result;
}


//获取网卡连接状态的函数
int getLinkStatus(const std::string& interfaceName) {
    std::stringstream commandStream;
    commandStream << "sudo ethtool " << interfaceName;
    std::string command = commandStream.str();

    std::string ethtoolOutput = executeCommand(command.c_str());

    size_t linkDetectedPos = ethtoolOutput.find("Link detected:");
    int linkStatus = -1; // Default value indicating "not found"

    if (linkDetectedPos != std::string::npos) {
        size_t valueStartPos = linkDetectedPos + strlen("Link detected:");
        size_t valueEndPos = ethtoolOutput.find("\n", valueStartPos);
        if (valueEndPos != std::string::npos) {
            std::string linkStatusStr = ethtoolOutput.substr(valueStartPos, valueEndPos - valueStartPos);
            // Trim leading and trailing spaces
            size_t start = linkStatusStr.find_first_not_of(" \t");
            size_t end = linkStatusStr.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                linkStatusStr = linkStatusStr.substr(start, end - start + 1);
                // Check if link is detected
                if (linkStatusStr == "yes") {
                    linkStatus = 1; // Link detected
                }
                else if (linkStatusStr == "no") {
                    linkStatus = 0; // Link not detected
                }
            }
        }
    }
    return linkStatus;
}


//本地win测试时，监听“http://localhost:8000/query/id”和"http://localhost:8000/query/topo"等,localhost:8080被我映射到虚拟机上去了
// 设备上测试时，IDQuery::IDQuery() : listener(L"http://localhost:8080/IDQuery")改为IDQuery::IDQuery() : listener(L"http://10.3.x.1/query/id")
// 其中'x'为事先确定好的，在哪台设备上，就填对应的‘x’，简单来说，填进去的应该是类似于"http://10.3.1.1/query/id"，而不是"http://10.3.x.1/query/id"
// 在虚拟机上，地址应该改为：“http://0.0.0.0:8080/query/id”，不能用localhost，不然会造成回环，而本地发送请求的POSTMAN和APIfox等，用“http://localhost:8080/query/id”,
// 我把本地的8080端口映射到虚拟机的8080端口上去了


//IDQuery
IDQuery::IDQuery() : listener(web::http::experimental::listener::http_listener(U("http://"+localhost+":8000/query/id"))) {               //改这里，就修改IDQuery监听的地址，实机测试时地址为：http://10.3.x.1/query/id
    listener.support(web::http::methods::GET, [this](web::http::http_request request) {
        HandleRequest(request);
        });

    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"IDQuery OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}


void IDQuery::HandleRequest(web::http::http_request request) {
    try {
        std::string ifconfigOutput = executeCommand("ifconfig");
        // 用ifconfigOutput来存储通过命令ifconfig所得到的信息

        size_t br0Pos = ifconfigOutput.find("br0");
        std::string xValue;

        if (br0Pos != std::string::npos) {
            // Find the position of 'inet' in the context of br0
            //size_t inetPos = ifconfigOutput.find("inet", br0Pos);
            size_t inetPos = ifconfigOutput.find("inet ", br0Pos);
            if (inetPos != std::string::npos) {
                // Find the position of the first digit after 'inet'
                size_t digitPos = ifconfigOutput.find_first_of("0123456789", inetPos);
                if (digitPos != std::string::npos) {
                    // Find the end position of the IP address
                    size_t endPos = ifconfigOutput.find(' ', digitPos);
                    if (endPos != std::string::npos) {
                        // Extract the IP address segment
                        std::string ipAddress = ifconfigOutput.substr(digitPos, endPos - digitPos);
                        // Splitting the IP address by '.' to extract the 'x' value
                        std::vector<std::string> ipAddressSegments;
                        size_t pos = 0;
                        std::string delimiter = ".";
                        while ((pos = ipAddress.find(delimiter)) != std::string::npos) {
                            ipAddressSegments.push_back(ipAddress.substr(0, pos));
                            ipAddress.erase(0, pos + delimiter.length());
                        }
                        // Check if we have at least one segment left (10.3.x.1 should have 4 segments)
                        if (ipAddressSegments.size() == 3) {
                            // The 'x' value is the third segment of the IP address
                            xValue = ipAddressSegments[2];
                        }
                    }
                }
            }
        }

        // 构建 JSON 响应
        web::json::value response;
        response[U("code")] = web::json::value::number(1);
        response[U("msg")] = web::json::value::string(U("success"));
        response[U("data")] = web::json::value::object({
            { U("DeviceID"), web::json::value::string(utility::conversions::to_string_t(xValue)) },
            { U("Status"), web::json::value::number(1) },
            { U("APIVersion"), web::json::value::string(U("1")) },
            { U("RouteVersion"), web::json::value::string(U("1")) }
            });


        // 回复请求
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        web::json::value errorResponse;
        errorResponse[U("code")] = web::json::value::number(0);
        errorResponse[U("msg")] = web::json::value::string(U("Internal server error"));
        request.reply(web::http::status_codes::InternalError, errorResponse);
    }
}




TopologyQuery::TopologyQuery() : listener(web::http::experimental::listener::http_listener(U("http://"+localhost+":8000/query/topo"))) {               //改这里，就修改TopologyQuery监听的地址,实机测试时地址为：http://10.3.x.1/query/topo
    listener.support(web::http::methods::GET, [this](web::http::http_request request) {
        HandleRequest(request);
        });

    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"TopologyQuery OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}

//构建融合拓扑，现在的融合拓扑含义为：拓扑矩阵中除了0元素，就是3位数，3位数的个位表示融合的连接情况type
//十位是mesh自组网电台的连接质量，百位是5G电台的连接质量
void TopologyQuery::HandleRequest(web::http::http_request request) {
    try {
        // 创建并复制 Meshtopo 和 FiveGtopo 矩阵
        int Meshtopo[routing_num][routing_num];
        int FiveGtopo[routing_num][routing_num];

        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                Meshtopo[i][j] = mesh_topu_source[i][j];
                FiveGtopo[i][j] = fiveG_topu_source[i][j];
            }
        }

        // 创建并复制 MeshQuality 和 FiveGQuality 矩阵
        int MeshQuality[routing_num][routing_num];
        int FiveGQuality[routing_num][routing_num];

        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                MeshQuality[i][j] = Meshtopo[i][j];
                FiveGQuality[i][j] = FiveGtopo[i][j];
            }
        }

        // 对 Meshtopo 和 FiveGtopo 进行修改
        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                Meshtopo[i][j] = (Meshtopo[i][j] != 0) ? 1 : 0;
                FiveGtopo[i][j] = (FiveGtopo[i][j] != 0) ? 2 : 0;
            }
        }

        // 对 Typetopo 进行计算
        int Typetopo[routing_num][routing_num];

        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                Typetopo[i][j] = Meshtopo[i][j] + FiveGtopo[i][j];
            }
        }

        // 对 MeshQuality, FiveGQuality, Typetopo 进行操作
        int RongHetopusource[routing_num][routing_num];

        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                RongHetopusource[i][j] = Typetopo[i][j] + MeshQuality[i][j] * 10 + FiveGQuality[i][j] * 100;
            }
        }
      
       // 将 RongHetopusource 转换为 JSON 数组
        web::json::value RongHetopusourceArray;
        for (int i = 0; i < routing_num; ++i) {
            for (int j = 0; j < routing_num; ++j) {
                 RongHetopusourceArray[i * routing_num + j] = web::json::value::number(RongHetopusource[i][j]);
            }
        }
       // 构建最终 JSON 响应
   web::json::value response;
  response[U("code")] = web::json::value::number(1);
  response[U("msg")] = web::json::value::string(U("success"));
  response[U("RongHetopo")] = RongHetopusourceArray;


        // 回复请求
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}



//SourceQuery源设备列表查询
struct ConnectionInfo {
    int type;
    std::string ip;
};

web::json::value createResponse(const std::unordered_map<std::string, std::string>& ipAddresses,
    const std::unordered_map<int, std::string>& typeToPcilan,
    const std::vector<ConnectionInfo>& connections) {
    web::json::value response;

    response[U("code")] = web::json::value::number(1);
    response[U("msg")] = web::json::value::string(U("success"));

    web::json::value data;
    data[U("count")] = web::json::value::number(connections.size());

    web::json::value connectionList;
    for (const auto& connection : connections) {
        web::json::value cdItem;
        cdItem[U("type")] = web::json::value::number(connection.type);
        cdItem[U("ip")] = web::json::value::string(utility::conversions::to_string_t(ipAddresses.at(typeToPcilan.at(connection.type))));
        connectionList[connection.type] = cdItem;
    }

    data[U("cdList")] = connectionList;
    response[U("data")] = data;

    return response;
}


SourceQuery::SourceQuery() : listener(web::http::experimental::listener::http_listener(U("http://"+localhost+":8000/query/source"))) {               //改这里，就修改SourceQuery监听的地址，实机测试时地址为：http://10.3.x.1/query/source
    listener.support(web::http::methods::GET, [this](web::http::http_request request) {
        HandleRequest(request);
        });

    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"SourceQuery OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}


void SourceQuery::HandleRequest(web::http::http_request request) {
    try {
        std::string ifconfigOutput = executeCommand("ifconfig");
        // 用ifconfigOutput来存储通过命令ifconfig所得到的信息
        std::vector<std::string> interfaceNames = { "vxlan0", "vxlan1", "vxlan2", "vxlan3" };
        std::unordered_map<std::string, std::string> ipAddresses;

        //提取接口 IP 地址
        for (const std::string& interfaceName : interfaceNames) {
            size_t startPos = ifconfigOutput.find(interfaceName);
            if (startPos != std::string::npos) {
                size_t inetPos = ifconfigOutput.find("inet ", startPos);
                if (inetPos != std::string::npos) {
                    size_t endPos = ifconfigOutput.find(" ", inetPos + 5); // Move to end of "inet "
                    if (endPos != std::string::npos) {
                        std::string ipAddress = ifconfigOutput.substr(inetPos + 5, endPos - inetPos - 5);
                        ipAddresses[interfaceName] = ipAddress;
                    }
                }
            }
        }

        // 定义 type 到 pcilan 的映射
        std::unordered_map<int, std::string> typeToPcilan = {
            {0, "vxlan0"},    //0是5G设备
            {1, "vxlan1"},    //1是mesh设备
            {2, "vxlan2"},
            {3, "vxlan3"}
            // 在此处添加更多的映射
        };

        // 现在，ipAddresses 中包含了对应接口名的 IP 地址。
        // 例如，ipAddresses["pcilan0"] 将包含 pcilan0 接口的 IP 地址。

        std::vector<ConnectionInfo> connections;
        int linkStatus0 = getLinkStatus("vxlan0");
        if (linkStatus0 != -1) {
            connections.push_back({ 0, ipAddresses["vxlan0"] });
        }

        int linkStatus1 = getLinkStatus("vxlan1");
        if (linkStatus1 != -1) {
            connections.push_back({ 1, ipAddresses["vxlan1"] });
        }

        int linkStatus2 = getLinkStatus("vxlan2");
        if (linkStatus2 != -1) {
            connections.push_back({ 2, ipAddresses["vxlan2"] });
        }

        int linkStatus3 = getLinkStatus("vxlan3");
        if (linkStatus3 != -1) {
            connections.push_back({ 3, ipAddresses["vxlan3"] });
        }

        web::json::value jsonResponse = createResponse(ipAddresses, typeToPcilan, connections);


        // Reply with JSON response
        request.reply(web::http::status_codes::OK, jsonResponse);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}

