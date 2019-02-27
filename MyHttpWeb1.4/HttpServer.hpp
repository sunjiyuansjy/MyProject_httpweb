#ifndef __HTTPSERVER_HPP__
#define __HTTPSERVER_HPP__

#include <iostream>
#include <pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"

class HttpServer{
  private:
    int listen_sock;
    int port;
    // ThreadPool pool;
  public:
    HttpServer(int port_):port(port_),listen_sock(-1)
  {}
    void InitServer()
    {
      listen_sock = SocketApi::Socket();
      SocketApi::Bind(listen_sock, port);
      SocketApi::Listen(listen_sock);
      // pool.InitThreadPool();
    }
    void Start()
    {
      for(;;){
        std::string peer_ip;
        int peer_port;
        //	int *sockp = new int;
        int sock = SocketApi::Accept(listen_sock, peer_ip, peer_port);
        if(sock >= 0){
          std::cout << peer_ip << ":" << peer_port << std::endl;
          Task t(sock,Entry::HandlerRequest);
          singleton::GetInsert()->PushTask(t);
          //	pthread_t tid;
          //	pthread_create(&tid, NULL, Entry::HandlerRequest, (void*)sockp);
        }
      }
    }
    ~HttpServer()
    {
      if(listen_sock >= 0){
        close(listen_sock);
      }
    }
};

#endif
