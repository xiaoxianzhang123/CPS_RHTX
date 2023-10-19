#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <json/json.h>
#include <curl/curl.h>

//10月12日
//编译方法：g++ get_data.cpp -o get_data -lcurl -I /usr/include/jsoncpp -ljsoncpp

using namespace std;

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

int main(int argc, char* argv[]) {
    char addr[40];
    //声明一个CURL句柄
    CURL* curl;
    //声明一个CURLcode类型的变量，存储CURL操作的返回代码
    CURLcode res;
    string response;
    if (2 < argc)
    {
        cout << "usage: get_topu <device id>" << endl;
        return 0;
    }
    sprintf(addr,"http://192.168.%s.9/query",argv[1]);
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
        return 0;
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
    int mesh_data[32][8];
    for(int i = 0; i < 32; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            mesh_data[i][j] = 0;
        }
    }
    cout << "提取的原始数据："<< endl;
        for (const auto& data : extractedData) {
        std::cout << "nodeId: " << data.first << std::endl;
        std::cout << "Data: ";
        int row = data.first;
         int j = 0;
        for (const auto& value : data.second) {
            mesh_data[row][j++] = value;
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
    cout << "保存到拓扑表的数据："<<endl;
    for(int i = 0; i < 32; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            cout << mesh_data[i][j] << " ";
        }
        cout << endl;
    }
    return 0;
}
