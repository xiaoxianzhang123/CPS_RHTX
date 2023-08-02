#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// 回调函数，用于处理接收到的HTTP响应数据
static size_t write_callback(void *contents, size_t size, size_t nmemb, char *output) {
    size_t total_size = size * nmemb;
    strcat(output, (char*)contents); // 将接收到的数据拼接到output字符串中
    return total_size;
}

int main(void) {
    CURL *curl;
    CURLcode res;

    // 接口地址
    char *url = "http://192.168.6.12/query";

    // 构造传输的参数
    char *data = "{\"FVersion\":\"?\"}";

    // 将参数拼接到URL中
    char url_with_params[256];
    snprintf(url_with_params, sizeof(url_with_params), "%s?data=%s", url, data);

    // 初始化CURL
    curl = curl_easy_init();
    if (curl) {
        char output[1024] = {0}; // 存储响应数据的缓冲区

        // 设置请求URL
        curl_easy_setopt(curl, CURLOPT_URL, url_with_params);

        // 设置响应回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, output);

        // 发起GET请求
        res = curl_easy_perform(curl);
        
        // 检查请求是否成功
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // 输出响应数据
            printf("HTTP Response:\n%s\n", output);
        }
        // 清理CURL
        curl_easy_cleanup(curl);
    }

    return 0;
}
