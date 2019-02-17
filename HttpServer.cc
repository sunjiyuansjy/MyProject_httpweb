#include <iostream>
#include "HttpServer.hpp"

//./HttpServer 8080
int main()
{

	if(argc !=2){
		Usage(argv[0]);
		exit(1);
	}
	HttpServer *ser = new HttpServer(atoi(argv[1])));
	ser->InitServer();
	ser->Start();
}
