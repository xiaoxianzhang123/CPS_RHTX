
#pragma once
#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <pplx/pplxtasks.h>
#include <mutex>
#include <map>
#include <chrono>
#include <thread>
#include <cstdio>
#include <array>

//��ѯ�࣬�����룬������Ľӿ�

class IDQuery {
public:
    IDQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

class BaseInfoQuery {
public:
    BaseInfoQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

class SourceQuery {
public:
    SourceQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

class TopologyQuery {
public:
    TopologyQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

class RoutingTableQuery {
public:
    RoutingTableQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

//���������࣬�д��û������������
//GeneralConfig
class GeneralConfig {
public:
    GeneralConfig();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};


//NetworkConfig
class NetworkConfig {
public:
    NetworkConfig();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};


//PhysicalInterfaceConfig
class PhysicalInterfaceConfig {
public:
    PhysicalInterfaceConfig();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};