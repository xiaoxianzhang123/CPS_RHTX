#pragma once
#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <array>
#include <atomic>


//启动函数
class HTTPServerManager {
public:
    HTTPServerManager() : serverRunning(false) {}

    void SetHttpsever(bool startServer);

private:
    std::atomic<bool> serverRunning;
};


//查询类，有输入，有输出的接口


//IDQuery，即ID查询
class IDQuery {
public:
    IDQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

//TopologyQuery拓扑查询
class TopologyQuery {
public:
    TopologyQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};


//SourceQuery源设备列表查询
class SourceQuery {
public:
    SourceQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};