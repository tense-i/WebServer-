all: epoll_web

epoll_web: epoll_web.c
	g++ -g -o epoll_web epoll_web.c

clean:
	rm -f epoll_web

.PHONY: all clean
