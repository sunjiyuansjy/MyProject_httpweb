#ifndef __HTTPSERVER_HPP__
#define __HTTPSERVER_HPP__

#include <iostream>
#include "ProtocolUtil.hpp"

class HttpServer{
	private:
		int listen_sock;
		int port;

	public:
		HttpServer(int port_):port(port_),listen_socket(-1)
	{}
		void InitServer()
		{
			listen_sock = SocketApi::Socket();
			SocketApi::Bind(listen_sock,port);
			SocketApi::Listen(listen_sock);
		}
		void *HandlerRequest(void *arg)
		{
			pthread_detach(pthread_self());
			int sock = *(int*)arg;
			delete arg;

			//for test
			char buf[10240];
			read(sock,buf,sizeof(buf));
			std::cout << buf << std:: endl;
		}
		void Start()
		{
			for(;;){
				std::string peer_ip;
				int peer_port;
				int sock = SocketApi::Accept(listen_sock,peer_ip,peer_port);
				if(sock >= 0){

					std::cout << peer_ip << ":" << peer_port << std::endl;
					//
					pthread_t tid;
					pthread_create(&tid, NULL, HandlerRequest,(void*)sock);

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
class Entry{

	public:
	void *HandlerRequest(void *arg)
	{
		pthread_detach(pthread_self());
		int sock = *(int*)arg;
		delete arg;

		//for test
		char buf[10240];
		read(sock,buf,sizeof(buf));
		std::cout << buf << std:: endl;
	}
};
#endif
