#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
int n;//记录自己的设备号

// 回调函数来处理HTTP响应
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    char *data = (char *)userp;
    strncat(data, (char *)contents, real_size);
    return real_size;
}

int main() {
    CURL *curl;
    CURLcode res;
    char response_data[4096] = "";  // 存储HTTP响应数据

    curl = curl_easy_init();
    if (curl) {
        // 设置HTTP请求的URL
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.29.155.4:8848/query");//修改了

        // 设置HTTP请求的方法
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // 设置HTTP请求头
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置HTTP请求参数
        const char *request_data = "{\"NodeId\":\"?\"}";//\不可少
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
            struct json_object *root, *node_id, *status,*chn0;
            root = json_tokener_parse(response_data);
            
            if (root) {
                json_object_object_get_ex(root, "NodeId", &node_id);
                json_object_object_get_ex(root, "Status", &status);
                json_object_object_get_ex(node_id, "chn0", &chn0);

                const char *node_id_str = json_object_to_json_string(node_id);
                const char *status_str = json_object_get_string(status);


                n = json_object_get_int(chn0);

                printf("NodeId: %s\n", node_id_str);
                printf("Status: %s\n", status_str);
                printf("chn0 value: %d\n", n);
                

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