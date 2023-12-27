#ifndef __HTTP_INTER_H
#define __HTTP_INTER_H


#pragma once
#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <array>
#include <atomic>


//��������
class HTTPServerManager {
public:
    HTTPServerManager() : serverRunning(false) {}

    void* SetHttpsever(bool startServer);

private:
    web::http::experimental::listener::http_listener listener;
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


//SourceQueryԴ�豸�б���ѯ
class SourceQuery {
public:
    SourceQuery();
    void HandleRequest(web::http::http_request request);

private:
    web::http::experimental::listener::http_listener listener;
};

// void HTTPServerManager::SetHttpserver(bool startServer);
// string executeCommand(const char* cmd);
// int getLinkStatus(const std::string& interfaceName);
// IDQuery::IDQuery() : listener(web::http::experimental::listener::http_listener(U("http://localhost:8000/query/id")));
// void IDQuery::HandleRequest(web::http::http_request request);
// TopologyQuery::TopologyQuery() : listener(web::http::experimental::listener::http_listener(U("http://localhost:8000/query/topo")));
// void TopologyQuery::HandleRequest(web::http::http_request request);
// web::json::value createResponse(const std::unordered_map<std::string, std::string>& ipAddresses,
//     const std::unordered_map<int, std::string>& typeToPcilan,
//     const std::vector<ConnectionInfo>& connections);
// SourceQuery::SourceQuery() : listener(web::http::experimental::listener::http_listener(U("http://localhost:8000/query/source")));
// void SourceQuery::HandleRequest(web::http::http_request request);



#endif