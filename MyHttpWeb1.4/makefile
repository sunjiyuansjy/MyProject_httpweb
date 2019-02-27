bin=HttpServer
cc=g++ -std=c++11
LDFLAGS = -lpthread

$(bin):HttpServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) 

.PHONY:clean
clean:
	rm -f $(bin)
