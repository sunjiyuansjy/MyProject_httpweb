bin=HttpServer
cc=g++
LDFLAGS = -lpthread

$(bin):HttpServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) -D_DEBUG_

.PHONY:clean
clean:
	rm -f $(bin)
