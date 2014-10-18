CC = g++
CFLAGS = -g -Wall -Wextra -Werror

SRCS = main.cpp socket.cpp server.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

server: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -fr *~ *.o server
