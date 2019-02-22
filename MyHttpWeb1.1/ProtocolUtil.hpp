#ifndef __PROTOCOLUTIL_HPP__
#define __PROTOCOLUTIL_HPP__

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define BUFF_NUM 1024

#define NORMAL 0
#define WARNING 1
#define ERROR 2

#define WEBROOT "wwwroot"
#define HOMEPATH "index.html"

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

class Connect{
	private:
		int sock;
	public:
		Connect(int sock_):sock(sock_)
	{}
		int RecvOneLine(std::string line_)
		{
			char buff[BUFF_NUM];
			int i = 0;
			char c = 'X';

			while(c!='\n' && i < BUFF_NUM - 1){
				ssize_t s = recv(sock,&c,1,0);
				if(s > 0){
					if(c=='\r')
					{
						recv(sock,&c,1,MSG_PEEK);
						if(c == '\n'){
							recv(sock,&c,1,0);
						}else{
							c = '\n';
						}
					}         
					// \r \n \r\n ->\n
					buff[i++] = c;
				}
				else{
					break;
				}
			}
			buff[i] = 0;
			line_ = buff;
			return i;
		}
		~Connect()
		{}

};

class Http_Request{
public:
	//基本协议字段
	std::string request_line;
	std::vector<std::string> request_header;
	std::string blank;
	std::string request_text;
	private:
	//解析协议字段
	std::string method;
	std::string uri;//path?arg
	std::string version;
	std::string path;
	std::string query_string;
	public:
	Http_Request():path(WEBROOT)
	{}
	void RequestLineParse()
	{
		//将一个字符串变成3个
		//GET /x/y/z /HTTP /1.1\n
		std::stringstream ss(request_line);
		ss >> method >> uri >> version;
		transform(method.begin(),method.end(),method.begin(),::toupper);
	}
	void UriParse()
	{
		if(method == "GET")
		{
			std::size_t pos = uri.find('?');
			if(pos!=std::string::npos){
				path += uri.substr(0,pos);
				query_string = uri.substr(pos+1);
			}else{
				//不带参数
				path += uri;//wwwroot/a/b/c/d.html
			}
		}else{
			 //POST
			 path += uri;
		}
		if(path[path.size()-1]=='/')
		{
			path += HOMEPATH;// wwwroot/index.html
		}
	}
	bool IsMethodLegal()
	{
		if(method!="GET"&&method!="POST")
		{
			return false;
		}
		return true;
	}
	~Http_Request()
	{}

};


class Entry{
	public:
		static void *HandlerRequest(void *arg)
		{
			pthread_detach(pthread_self());
			int sock = *(int*)arg;
			delete arg;

#ifdef _DEBUG_
			//for test
			char buf[10240];
			read(sock, buf, sizeof(buf));
			std::cout << "#####################" << std::endl;
			std::cout << buf << std::endl;
			std::cout << "#####################" << std::endl;
#else
			Connect *conn = new Connect(sock);
			Http_Request *rq = new Http_Request;

			conn->RecvOneLine(rq->request_line);
			rq->RequestLineParse();
			if(rq->IsMethodLegal())
			{
				LOG("Request Method Is not Legal",WARNING);
				goto end;
			}
rq->UriParse();		
end:
#endif

			close(sock);
			return (void *)0;
		}
};










#endif

