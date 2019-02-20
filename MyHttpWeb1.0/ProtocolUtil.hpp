#ifndef __PROTOCOLUTIL_HPP__
#define __PROTOCOLUTIL_HPP__

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 5

#define NORMAL 0
#define WARNING 1
#define ERROR 2

const char *ErrLevel[]={
	"Normal",
	"Warning",
	"Error"
};
//__FILE__ ___LINE__
void log(std::string msg,int level,std::string file,int line)
{
	std::cout << file << ":" << line <<"." << msg <<" "<< ErrLevel[level] << std::endl;
}

#define LOG(msg,level) log(msg,level,__FILE__, __LINE__);

class SocketApi{
	public:
	static int Socket()
		{
			int sock = socket(AF_INET,SOCK_STREAM,0);
			if(sock < 0){
				LOG("socket error!",ERROR);
				exit(2);
			}
			int opt = 1;
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			return sock;
		}
		static void Bind(int sock,int port)
		{
			struct sockaddr_in local;
			bzero(&local,sizeof(local));
			local.sin_family = AF_INET;
			local.sin_port = htons(port);
			local.sin_addr.s_addr = htonl(INADDR_ANY);

			if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0){
				LOG("bind error!",ERROR);
				exit(3);
			}
		}
		static void Listen(int sock)
		{
			if(listen(sock,BACKLOG) < 0)
			{
				LOG("listen error!",ERROR);
				exit(4);
			}
		}
		static int Accept(int listen_sock,std::string &ip,int &port)
		{
			struct sockaddr_in peer;
			socklen_t len = sizeof(peer);
			int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
			if(sock<0){
				LOG("accept error",WARNING);
				return -1;
			}
			port = ntohs(peer.sin_port);
			ip = inet_ntoa(peer.sin_addr);
			return sock;
	}

};

class Entry{
	public:
		static void *HandlerRequest(void *arg)
		{
			pthread_detach(pthread_self());
			int sock = *(int*)arg;
			delete arg;

			//for test
			char buf[10240];
			read(sock, buf, sizeof(buf));
			std::cout << "#####################" << std::endl;
			std::cout << buf << std::endl;
			std::cout << "#####################" << std::endl;
			close(sock);
			return (void *)0;
		}
};










#endif

