CC = g++
CFLAGS = -g -Wall -Wextra -Werror

HOST_SRCS = host.cpp GbnProtocol.cpp
HOST_OBJS = $(subst .cpp,.o,$(HOST_SRCS))

all: host

host: $(HOST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(HOST_OBJS)

clean:
	rm -fr *~ *.o host client
