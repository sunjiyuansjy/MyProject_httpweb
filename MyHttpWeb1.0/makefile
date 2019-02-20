bin=HttpServer
cc=g++
LDFLAGS = -lpthread

$(bin):HttpServer.cc
	$(cc) -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	rm -f $(bin)
