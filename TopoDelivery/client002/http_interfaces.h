#pragma once
#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <array>
#include <atomic>


//��������
class HTTPServerManager {
public:
    HTTPServerManager() : serverRunning(false) {}

    void SetHttpsever(bool startServer);

private:
    std::atomic<bool> serverRunning;
};


//��ѯ�࣬�����룬������Ľӿ�


//IDQuery����ID��ѯ
class IDQuery {
public:
    IDQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

//TopologyQuery���˲�ѯ
class TopologyQuery {
public:
    TopologyQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};


//SourceQueryԴ�豸�б��ѯ
class SourceQuery {
public:
    SourceQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};