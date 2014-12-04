CC = g++
CFLAGS = -g -O0 -Wall -Wextra -Werror

HOST_SRCS = host.cpp GbnProtocol.cpp
HOST_OBJS = $(subst .cpp,.o,$(HOST_SRCS))

CLIENT_SRCS = client.cpp GbnProtocol.cpp
CLIENT_OBJS = $(subst .cpp,.o,$(CLIENT_SRCS))

all: server receiver

server: $(HOST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(HOST_OBJS)
	
receiver: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS)

clean:
	rm -fr *~ *.o server receiver
