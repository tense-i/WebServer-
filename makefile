all: event_web

event_web: event_web.c
	g++ -g -o event_web event_web.c

clean:
	rm -f event_web

.PHONY: all clean
