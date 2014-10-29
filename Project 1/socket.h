#ifndef socket_head
#define socket_head
#define MAX_REQUEST_SIZE 8192

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

using namespace std;

class Socket {
	public:
		Socket();
		~Socket();

		bool create();
		bool bind(int port);
		bool listen(int backlog);
		bool closes();
		bool accept(Socket& socket);
		bool send(const char *data);
		bool receive(string& data);
		int portNumber();
	private:
		int socketFd;
		sockaddr_in sockAddr;
};

#endif

