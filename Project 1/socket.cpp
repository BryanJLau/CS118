#include "socket.h"

Socket::Socket() {
	socketFd = -1; // Invalid fd
    memset(&sockAddr, 0, sizeof(sockAddr)); // Zero out the struct
}

// Close the socket if it's still open
Socket::~Socket() {
	if(socketFd != -1) close(socketFd);
}

bool Socket::closes() {
	cout << "closing socket" << endl;
	close(socketFd);
}

bool Socket::create() {
	// Create the socket
	socketFd = socket(AF_INET, SOCK_STREAM, 0);

	// True if the socket was created successfully
    return socketFd != -1;
}

bool Socket::bind(int port) {
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons(port);

    return (::bind(socketFd, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) == 0);
}

bool Socket::accept( Socket& socket ) {
	int length = sizeof(sockAddr);
    socket.socketFd = ::accept(
    	socketFd, 
    	(sockaddr*) &sockAddr,
    	 (socklen_t*) &length);

    return socket.socketFd != -1;
}

bool Socket::listen(int backlog) {
    return ::listen(socketFd, backlog) == 0;
}

bool Socket::send(const char * data) {
    return ::send(socketFd, data,strlen(data)+1, MSG_OOB) == 0;
}

bool Socket::receive(string& data) {
    char buffer[MAX_REQUEST_SIZE + 1];
    memset(buffer, '\0', sizeof(buffer));

    recv(socketFd, buffer, MAX_REQUEST_SIZE, 0) > 0 ?
    	data = buffer : data = "";
	return data != "";
}

int Socket::portNumber() {
    sockaddr_in addr;
    socklen_t length = sizeof(addr);
    if(getsockname(socketFd, (struct sockaddr*) &addr, &length) == -1)
    	// Socket isn't bound
        return -1;
    else
        return ntohs(addr.sin_port);
}

