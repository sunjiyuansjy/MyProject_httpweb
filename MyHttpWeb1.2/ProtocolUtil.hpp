#ifndef __PROTOCOLUTIL_HPP__
#define __PROTOCOLUTIL_HPP__

#include <iostream>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define BACKLOG 5
#define BUFF_NUM 1024

#define NORMAL 0
#define WARNING 1
#define ERROR 2

#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"

const char *ErrLevel[]={
  "Normal",
  "Warning",
  "Error"
};
//__FILE__ ___LINE__
void log(std::string msg,int level,std::string file,int line)
{
  std::cout << " [ "<< file << ":" << line <<"]" << msg <<" [ "<< ErrLevel[level] <<" ] "<< std::endl;
}

#define LOG(msg,level) log(msg,level,__FILE__, __LINE__);


class Util{
  public:
    static void MakeKV(std::string s,std::string &k,std::string &v)
    {
      std::size_t pos = s.find(":");
      k = s.substr(0,pos);
      v = s.substr(pos+2);
    }
    static std::string IntToString(int &x)
    {
      std::stringstream ss;
      ss << x;
      return ss.str();

    }
    static std::string CodeToDosc(int code)
    {
      switch(code)
      {
        case 200:
          return "OK";

        case 404:
          return "Not Found";
        default:
          break;
      }
      return "Unknow error";
    }
};
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

class Http_Response{
  public:
    //基本协议字段
    std::string status_line;
    std::vector<std::string> response_header;
    std::string blank;
    std::string response_text;
  private:
    int code;
    std::string path;
    int recource_size;
  public:
    Http_Response()
      :blank("\r\n"),code(200),recource_size(0)
    {}
    int &Code()
    {
      return code;
    }
    void SetPath(std::string &path_)
    {
      path = path_;
    }
    std::string &Path()
    {
      return path;
    }
    void SetRecourceSize(int rs)
    {
      recource_size = rs;
    }
    int RecourceSize()
    {
      return recource_size;
    }
    void MakeStatusLine()
    {
      status_line = "HTTP/1.0";
      status_line = " ";
      status_line = Util::IntToString(code);
      status_line = " ";
      status_line = Util::CodeToDosc(code);
      status_line = "\r\n";
      LOG("Make Status Line Done!",NORMAL);
    }
    void MakeResponseHeader()
    {
      std::string line;
      line = "Content-Length:";
      line += Util::IntToString(recource_size);
      line +="\r\n";
      response_header.push_back(line);
      line += "\r\n";
      response_header.push_back(line);
      LOG("Make Response Header Done!",NORMAL);
    }
    ~Http_Response()
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
    //int recource_size;
    std::string query_string;
    std::unordered_map<std::string,std::string> header_kv;
    bool cgi;
  public:
    Http_Request():path(WEBROOT),cgi(false),blank("\r\n")
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
          cgi = true;
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
        path += HOMEPAGE; // wwwroot/index.html
      }
    }
    void HeaderParse()
    {
      std::string k,v;
      for(auto it=request_header.begin();it!=request_header.end();it++)
      {
        Util::MakeKV(*it,k,v);
        header_kv.insert({k,v});
      }
    }

    bool IsMethodLegal()
    {
      if(method != "GET"&& method != "POST")
      {
        return false;
      }
      return true;
    }

    int IsPathLegal(Http_Response *rsp)// wwwroot/a/b/c/d.html
    {
      int rs = 0; 
      struct stat st;
      if(stat(path.c_str(),&st)<0)
      {
        LOG("file is not exist!",WARNING);
        return 404;
      }
      else{
        rs = st.st_size;
        if(S_ISDIR(st.st_mode)){
          path += "/";
          path += HOMEPAGE;
          stat(path.c_str(),&st);
          rs = st.st_size;
        }else if((st.st_mode & S_IXUSR) ||
            (st.st_mode & S_IXGRP) ||
            (st.st_mode & S_IXOTH)){
          cgi = true;
        }else{
          //TODO
        }
      }
      rsp->SetPath(path);
      rsp->SetRecourceSize(rs);
      LOG("PATH is ok!",NORMAL);
      return 0;
    }
    bool IsNeedRecv()
    {
      return method == "POST" ? true:false;
    }
    bool IsCgi()
    {
      return cgi;
    }
    int ContentLength()
    {
      int content_length= -1;
      std::string cl = header_kv["Content-Line"];
      std:: stringstream ss(cl);
      ss >> content_length;
      return content_length;
    }
    ~Http_Request()
    {}

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
    void RecvRequestHeader(std::vector<std::string> &v)
    {
      std::string line = "X";
      while(line != "\n"){
        RecvOneLine(line);
        if(line != "\n")
        {
          v.push_back(line);
        }
      }
      LOG("Header Recv OK!",NORMAL);
    }
    void RecvText(std::string &Text,int content_length)
    {
      char c;
      for(auto i=0;i<content_length;i++)
      {
        recv(sock,&c,1,0);
        Text.push_back(c);
      }
    }
    void SendStatusLine(Http_Response *rsp)
    {
      std::string &sl = rsp->status_line;
      send(sock,sl.c_str(),sl.size(),0);
    }
    void SendHeader(Http_Response *rsp)
    {
      std::vector<std::string> &v = rsp->response_header;
      for(auto it = v.begin();it != v.end();it++)
      {
        send(sock,it->c_str(),it->size(),0);
      }
    }
    void SendText(Http_Response *rsp)
    {
      std::string &path = rsp -> Path();
      int fd = open(path.c_str(),O_RDONLY);
      if(fd<0){
        LOG("open file error!",WARNING);
      return;
      }
    sendfile(sock,fd,NULL,rsp->RecourceSize());
    close(fd);
    }
    ~Connect()
    {
      close(sock);
    }

};

class Entry{
  public:
    static void ProcessNonCgi(Connect *conn,Http_Request *rq,Http_Response *rsp)
    {
      rsp->MakeStatusLine();
      rsp->MakeResponseHeader();
      // rsp->MakeResponseText(rq);

      conn->SendStatusLine(rsp);
      conn->SendHeader(rsp);//add \n
      conn->SendText(rsp);
      LOG("Send Response Done!",NORMAL);
    }
    static void ProcessResponse(Connect *conn,Http_Request *rq,Http_Response *rsp)
    {
      if(rq->IsCgi()){
        //ProcesCgi();

      }else{
        LOG("MakeResponse Use Non Cgi!",NORMAL);
        ProcessNonCgi(conn,rq,rsp);
      }
    }
    static void *HandlerRequest(void *arg)
    {
      pthread_detach(pthread_self());
      int *sock = (int*)arg;
      delete arg;

#ifdef _DEBUG_
      //for test
      char buf[10240];
      read(*sock, buf, sizeof(buf));
      std::cout << "#####################" << std::endl;
      std::cout << buf << std::endl;
      std::cout << "#####################" << std::endl;
#else
      Connect *conn = new Connect(*sock);
      Http_Request *rq = new Http_Request;
      Http_Response *rsp = new Http_Response;
      int &code=rsp->Code();
      conn->RecvOneLine(rq->request_line);
      rq->RequestLineParse();
      if(rq->IsMethodLegal())
      {
        LOG("Request Method Is not Legal",WARNING);
        goto end;
      }
      rq->UriParse();		
      if(rq->IsPathLegal(rsp)!=0)
      {
        code=404;
        LOG("file is not exist!",WARNING);
        goto end;
      }
      conn ->RecvRequestHeader(rq->request_header);
      rq->HeaderParse();
      if(rq->IsNeedRecv())
      {
        conn->RecvText(rq->request_text,rq->ContentLength());
        LOG("POST Method,Need Rev Begin!",NORMAL);
      }
      LOG("Http Request Recv Done,OK!",NORMAL);
      ProcessResponse(conn,rq,rsp);
end:
      delete conn;
      delete rq;
      delete rsp;
      delete sock;
#endif

      return (void *)0;
    }
};

#endif

