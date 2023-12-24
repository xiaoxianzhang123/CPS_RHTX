//待验证，验证成功之后，将get_test.c和get_topo.c进行结合，主要就是将get_test.c得到的n值用作get_topo.c的查询输入。目前已验证，
//get_test.c可以和模拟的接口进行请求和返回。get_topo.c中对一个D可以解析数据。

#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>

// // 定义连接状态的位域偏移
// #define SIGNAL_STRENGTH_BIT_OFFSET(nodeId, neighborId) ((nodeId) * 8 + (neighborId))

// 回调函数来处理HTTP响应
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    char *data = (char *)userp;
    strncat(data, (char *)contents, real_size);
    return real_size;
}

// 数据结构表示邻居的连接状态
typedef struct {
    int signalStrength;  // 信号强度，可以根据需要选择合适的数据类型
    // 其他可能需要的邻居信息
} Neighbor;

// 使用一个数据结构存储邻居信息
    Neighbor neighbors[40];  // 8*4个邻居状态

int main() {
    CURL *curl;
    CURLcode res;
    char response_data[4096] = "";  // 存储HTTP响应数据

    curl = curl_easy_init();
    if (curl) {
        // 设置HTTP请求的URL
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.z.9/query");//地址

        // 设置HTTP请求的方法
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // 设置HTTP请求头
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置HTTP请求参数
        const char *request_data = "{\"topo\":3}";  // 下级子网编号，可以根据需要修改
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);

        // 设置写回调函数，将HTTP响应数据写入response_data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_data);

        // 发送HTTP请求
        res = curl_easy_perform(curl);

        // 检查HTTP请求是否成功
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // 解析JSON响应
            struct json_object *root, *topo, *info, *nodeArray;
            root = json_tokener_parse(response_data);

            if (root) {
                json_object_object_get_ex(root, "topo", &topo);
                json_object_object_get_ex(topo, "info", &info);
                nodeArray = json_object_get_array(info);

                // 寻找Nodeld=n的拓扑信息
                int nIndex = -1;
                char targetNodeId[10];
                int n=3;//自己的NodeId
                sprintf(targetNodeId, "N%d", n);  // 将整数转换为字符串
                for (int i = 0; i < json_object_array_length(nodeArray); i++) 
                {
                    struct json_object *node = json_object_array_get_idx(nodeArray, i);
                    struct json_object *nodeIdObj;
                    json_object_object_get_ex(node, "nodeId", &nodeIdObj);
                    const char *nodeId = json_object_get_string(nodeIdObj);

                    if (strcmp(nodeId, targetNodeId) == 0) {
                        nIndex = i;
                        break;
                    }
                }
                if (nIndex != -1) {
                    // 找到Nodeld=n的拓扑信息，继续处理

                    struct json_object *nNode = json_object_array_get_idx(nodeArray, nIndex);
                    struct json_object *nNeighbors;
                    json_object_object_get_ex(nNode, "node", &nNeighbors);

                    
                    int neighbor_n=0;
                    // 提取连接状态信息
                    for (int i = 0; i < 8; i++) //8个D
                    {
                        json_object *D = json_object_array_get_idx(nNeighbors, i);//获得D
                        const char *D_string = json_object_get_string(D);//D的类型为字符，需要转换为整数
                        int D_int = atoi(D_string);
                        int mark=3;
                        for(int j=0;j<=3;j++)
                        {
                            neighbors[neighbor_n++].signalStrength = (D_int>>(j*2))&mark;//获取设备的信号强度
                        }
                        
                    }

                    // 输出邻居信息
                    printf("Nn节点的邻居信息：\n");
                    for (int i = 0; i < 24; i++) {
                        printf("邻居%d的信号强度：%d\n", i, neighbors[i].signalStrength);
                    }
                } else {
                    printf("未找到Nodeld=n的拓扑信息\n");
                }

                

                json_object_put(root);  // 释放JSON对象
            } else {
                fprintf(stderr, "Failed to parse JSON response\n");
            }
        }

        // 清理CURL资源
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}
