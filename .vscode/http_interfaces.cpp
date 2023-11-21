//兼容性
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
#include <pplx/pplxtasks.h>
#include <mutex>
#include <map>
#include <chrono>
#include <thread>
#include <array>


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


//本地测试时，监听“http://localhost:8080/IDQuery”和"http://localhost:8080/BaseInfoQuery"等
//IDQuery
IDQuery::IDQuery() : listener(L"http://localhost:8080/IDQuery") {
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

//模版2的IDQuery，功能是让客户端自己ifconfig获取ID信息，与下面的选一个启用
/*
void IDQuery::HandleRequest(web::http::http_request request) {
    try {
        std::string ifconfigOutput = executeCommand("ifconfig");
        // 解析 ifconfigOutput，获取所需的设备信息
        // 假设从 ifconfig 的输出中提取了设备信息，并将其存储在 deviceInfo 变量中

        // 构建 JSON 响应
        web::json::value response;
        response[U("DeviceID")] = web::json::value::string(deviceInfo);
        response[U("Status")] = web::json::value::string("OK");

        // 回复请求
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}
*/

//模版1的IDQuery，功能是从另一个http接口获取信息并传送回客户端，与上面的选一个启用
/*
void IDQuery::HandleRequest(web::http::http_request request) {
    try {
        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            inputParam = query[U("inputParam")];
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);

        // 发送请求到另一个 HTTP 接口
        web::http::client::http_client client(U("http://192.168.z.9/query"));

        // 构建请求 JSON
        web::json::value requestBody;
        requestBody[U("NodeId")] = web::json::value::string(U("?")); // 设置NodeId为?

        // 发送 POST 请求
        client.request(web::http::methods::POST, U(""), requestBody)
            .then([=](web::http::http_response response) {
            if (response.status_code() == web::http::status_codes::OK) {
                return response.extract_json();
            }
            else {
                throw std::runtime_error("Failed to get data from the remote server");
            }
                })
            .then([=](web::json::value responseBody) {
                    // 解析响应 JSON
                    int chn0 = responseBody[U("NodeId")][U("chn0")].as_integer();
                    int chn1 = responseBody[U("NodeId")][U("chn1")].as_integer();
                    utility::string_t status = responseBody[U("Status")].as_string();

                    // 构建 IDQuery 的响应 JSON
                    web::json::value idQueryResponse;
                    idQueryResponse[U("DeviceID")] = web::json::value::number(chn0);
                    idQueryResponse[U("Status")] = web::json::value::string(status);

                    // 回复请求
                    request.reply(web::http::status_codes::OK, idQueryResponse);
                })
                    .wait(); // 等待请求完成
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}
*/

//测试用的IDQuery，参数都被固定死了
void IDQuery::HandleRequest(web::http::http_request request) {
    try {
        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            inputParam = query[U("inputParam")];
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);

        // 构建 JSON 响应
        web::json::value response;
        response[U("DeviceID")] = web::json::value::number(2);            //改这里面的tmp，则改变了DeviceID,比如“web::json::value::number(2);”则DeviceID变为2
        response[U("Status")] = web::json::value::string(U("OK"));              //这里的Status已经默认为OK
        response[U("APIVersion")] = web::json::value::string(U("1.0"));         //写死为1.0
        response[U("RouteVersion")] = web::json::value::string(U("1.0"));         //写死为1.0

        // 回复请求
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        // 捕获并处理任何异常
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}


//BaseInfoQuery
BaseInfoQuery::BaseInfoQuery() : listener(L"http://localhost:8080/BaseInfoQuery") {
    listener.support(web::http::methods::GET, [this](web::http::http_request request) {
        HandleRequest(request);
        });
    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"BaseInfoQuery OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}



void BaseInfoQuery::HandleRequest(web::http::http_request request) {
    try {


        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            if (!query[U("inputParam")].empty()) {
                inputParam = query[U("inputParam")];
            }
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);

        web::json::value response;
        response[U("DeviceID")] = web::json::value::number(tmp);
        response[U("Loglevel")] = web::json::value::number(tmp);
        response[U("LogOutputMode")] = web::json::value::number(tmp);
        response[U("APIVersion")] = web::json::value::number(tmp);
        response[U("RouteVersion")] = web::json::value::number(tmp);
        response[U("RoutingProtocol")] = web::json::value::number(tmp);

        request.reply(web::http::status_codes::OK, response);

    }
    catch (const std::exception& ex) {
        // 异常处理代码
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}

//SourceQuery源设备列表查询
SourceQuery::SourceQuery() : listener(L"http://localhost:8080/SourceQuery") {
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


//测试和演示的数据定死的版本，写死的为6个Type和其对应的num，ip等，可以轻松修改
void SourceQuery::HandleRequest(web::http::http_request request) {
    try {


        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            if (!query[U("inputParam")].empty()) {
                inputParam = query[U("inputParam")];
            }
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);

        // 构建节点拓扑的 JSON 结构
        web::json::value sourceListArray = web::json::value::array();
        // 模拟数据
        std::vector<int> types = { 1, 2, 3, 4, 5, 6 };
        std::vector<int> nums = { 0, 1, 2, 3, 4, 5 };
        std::vector<std::string> ips = {"10.2.1.2","10.3.2.4","10.1.5.6",
                                        "10.5.2.1","10.1.2.1","10.2.1.2"};

        // 根据模拟数据构建 SourceList 数组
        for (size_t i = 0; i < types.size(); ++i) {
            web::json::value sourceObject;

            // 构建每个对象的键值对
            sourceObject[U("Type")] = web::json::value::number(types[i]);
            sourceObject[U("Num")] = web::json::value::number(nums[i]);
            sourceObject[U("IP")] = web::json::value::string(utility::conversions::to_string_t(ips[i]));

            // 将对象添加到数组中
            sourceListArray[i] = sourceObject;
        }

        // 构建完整的响应 JSON 结构
        web::json::value response;
        response[U("SourceList")] = sourceListArray;

        // 发送响应
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        // 异常处理代码
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}

//TopologyQuery拓扑查询
TopologyQuery::TopologyQuery() : listener(L"http://localhost:8080/TopologyQuery") {
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


//测试演示版本TopologyQuery，数据都写死了，没有数据输入。写死的数据为：7个node_id和对应的node，可轻松修改
void TopologyQuery::HandleRequest(web::http::http_request request) {
    try {


        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            if (!query[U("inputParam")].empty()) {
                inputParam = query[U("inputParam")];
            }
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);


        // 构建节点拓扑的 JSON 结构
        web::json::value nodeTopologyArray = web::json::value::array();

        // 临时的节点 ID 和节点信息数据（示例）
        std::vector<std::string> nodeIDs = { "N0", "N1", "N2", "N3", "N4", "N5", "N6" };
        std::vector<std::vector<int>> nodeInfos = {
            {0, 1, 2, 3, 1, 2, 0, 3},
            {3, 2, 1, 0, 2, 3, 1, 0},
            {1, 0, 3, 2, 0, 1, 3, 2},
            {2, 3, 0, 1, 3, 0, 2, 1},
            {0, 1, 2, 3, 1, 2, 0, 3},
            {3, 2, 1, 0, 2, 3, 1, 0},
            {1, 0, 3, 2, 0, 1, 3, 2}
        };

        // 根据数据构建节点拓扑的 JSON 结构
   // 根据数据构建节点拓扑的 JSON 结构
        for (size_t i = 0; i < nodeIDs.size(); ++i) {
            web::json::value nodeObject;
            nodeObject[U("nodeId")] = web::json::value::string(utility::conversions::to_string_t(nodeIDs[i]));

            // 构建节点数组
            web::json::value nodeArray = web::json::value::array();
            // 向节点数组添加节点信息
            for (int j = 0; j < 8; ++j) {
                nodeArray[j] = web::json::value::number(nodeInfos[i][j]);
            }

            // 将节点数组添加到节点对象中
            nodeObject[U("node")] = nodeArray;

            // 将节点对象添加到节点拓扑数组中
            nodeTopologyArray[i] = nodeObject;
        }


        // 构建完整的响应 JSON 结构
        web::json::value response;
        response[U("NodeTopology")] = nodeTopologyArray;

        // 发送响应
        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& ex) {
        // 异常处理代码
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}



//RoutingTableQuery
RoutingTableQuery::RoutingTableQuery() : listener(L"http://localhost:8080/RoutingTableQuery") {
    listener.support(web::http::methods::GET, [this](web::http::http_request request) {
        HandleRequest(request);
        });
    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"RoutingTableQuery OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}



void RoutingTableQuery::HandleRequest(web::http::http_request request) {
    try {


        // 获取查询参数 "inputParam"
        utility::string_t inputParam = U("0"); // 默认值为 "0"
        if (!request.absolute_uri().query().empty()) {
            auto query = web::http::uri::split_query(request.absolute_uri().query());
            if (!query[U("inputParam")].empty()) {
                inputParam = query[U("inputParam")];
            }
        }

        // 将 inputParam 转换为整数（假设它是整数）
        int tmp = std::stoi(inputParam);

        web::json::value response;
        response[U("RoutingTable")] = web::json::value::number(tmp);
        response[U("TerminalDevicesList")] = web::json::value::number(tmp);

        request.reply(web::http::status_codes::OK, response);

    }
    catch (const std::exception& ex) {
        // 异常处理代码
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}



////参数配置类，有从用户处输入有输出
//GeneralConfig
GeneralConfig::GeneralConfig() : listener(L"http://localhost:8080/GeneralConfig") {
    listener.support(web::http::methods::POST, [this](web::http::http_request request) {
        HandleRequest(request);
        });
    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"GeneralConfig OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}




void GeneralConfig::HandleRequest(web::http::http_request request) {
    try {

        // 默认值为 "0"
        int tmp = 0;

        // 提取请求中的 JSON 数据
        request.extract_json().then([=, &tmp](web::json::value requestBody) {
            // 检查请求体中是否存在 "inputParam" 字段
            if (requestBody.has_field(U("inputParam"))) {
                // 如果存在 "inputParam" 字段，将其值转换为整数并赋给 tmp
                tmp = requestBody[U("inputParam")].as_integer();
            }

            // 创建 JSON 响应
            web::json::value response;
            response[U("Result")] = web::json::value::number(tmp);

            // 回复请求，并记录 IP 地址到访问记录中
            request.reply(web::http::status_codes::OK, response);

            }).wait(); // 等待提取 JSON 完成
    }
    catch (const std::exception& ex) {
        // 捕获并处理任何异常
        std::cerr << "Exception caught: " << ex.what() << std::endl;

        // 返回内部服务器错误响应
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}




//NetworkConfig
NetworkConfig::NetworkConfig() : listener(L"http://localhost:8080/NetworkConfig") {
    listener.support(web::http::methods::POST, [this](web::http::http_request request) {
        HandleRequest(request);
        });
    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"NetworkConfig OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}


//命令行版NetworkConfig，可调用NetworkConfig更改客户端IP
void NetworkConfig::HandleRequest(web::http::http_request request) {
    try {
        web::json::value requestJson = request.extract_json().get();

        // 检查请求中是否包含名为 "IP" 的字段，并且该字段的类型是字符串
        if (requestJson.has_field(U("IP")) && requestJson[U("IP")].is_string()) {
            utility::string_t ipAddress = requestJson[U("IP")].as_string();

            // 构建设置 IP 地址的命令
            std::string command = "sudo ip addr add " + utility::conversions::to_utf8string(ipAddress) + "/24 dev eth0";

            // 执行命令
            std::string result = executeCommand(command.c_str());


            // 构建响应 JSON
            web::json::value responseJson;
            responseJson[U("IP")] = web::json::value::string(ipAddress);

            // 回复请求
            request.reply(web::http::status_codes::OK, responseJson);
        }

        // 如果无法从请求中获取有效的 IP 地址，则返回错误响应
        request.reply(web::http::status_codes::BadRequest, U("Invalid or missing IP parameter"));
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}
/*
        // 构建设置 IP 地址的命令
       std::string command = "sudo ip addr add " + utility::conversions::to_utf8string(ipAddress) + "/24 dev eth0";

        // 执行命令
        std::string result = executeCommand(command.c_str());


        // 构建响应 JSON
        web::json::value responseJson;
        responseJson[U("IP")] = web::json::value::string(ipAddress);

        // 回复请求
        request.reply(web::http::status_codes::OK, responseJson);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}
*/


//PhysicalInterfaceConfig
PhysicalInterfaceConfig::PhysicalInterfaceConfig() : listener(L"http://localhost:8080/PhysicalInterfaceConfig") {
    listener.support(web::http::methods::POST, [this](web::http::http_request request) {
        HandleRequest(request);
        });
    try {
        listener.open().wait(); // 直接调用 open() 并等待其完成
        std::wcout << L"PhysicalInterfaceConfig OK" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught while opening listener: " << ex.what() << std::endl;
    }
}




void PhysicalInterfaceConfig::HandleRequest(web::http::http_request request) {
    try {

        // 默认值为 "0"
        int tmp = 0;

        request.extract_json().then([=, &tmp](web::json::value requestBody) {
            if (requestBody.has_field(U("inputParam"))) {
                tmp = requestBody[U("inputParam")].as_integer();
            }

            web::json::value response;
            response[U("Result")] = web::json::value::number(tmp);

            request.reply(web::http::status_codes::OK, response);

            }).wait(); // 等待提取JSON完成
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        request.reply(web::http::status_codes::InternalError, U("Internal server error"));
    }
}


//主函数 main 来启动应用程序并初始化处理程序类，然后开始监听HTTP请求
int main() {
    // 启动HTTP服务器监听器的线程
    std::thread httpThread([]() {
        //监听IDQuery端口
        IDQuery idQueryHandler;

        //监听BaseInfoQuery端口
        BaseInfoQuery baseQueryHandler;


        //监听SourceQuery端口
        SourceQuery sourceQueryHandler;


        //监听TopologyQuery端口
        TopologyQuery topologyQueryHandler;


        //监听RoutingTableQuery端口
        RoutingTableQuery routingtableQueryHandler;


        //监听GeneralConfig端口
        GeneralConfig generalConfigHandler;

        //监听NetworkConfig端口
        NetworkConfig networkConfigHandler;


        //监听PhysicalInterfaceConfig端口
        PhysicalInterfaceConfig physicalinterfaceConfigHandler;


        while (true) {

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        });
    httpThread.join(); // 等待HTTP服务器监听器线程结束
    return 0;
}
