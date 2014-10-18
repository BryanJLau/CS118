#ifndef server_head
#define server_head

#include <string>
#include <set>
#include "socket.h"

#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 NOT FOUND"
#define MAX_CONNECTIONS 256

using namespace std;

class Server {
	public:
		Server(int port, string root);
		~Server();

		void freeChildren(int signal); // Frees children on signal
		void freeChild(pid_t p); // Free child memory after it exits
		void listen();
		void handleRequest(Socket& responseSocket, const string& request);
	private:
		int portNumber;
		string docRoot;
		set<pid_t> childForks;
		Socket socket;
};

#endif

