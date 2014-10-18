#include "server.h"
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>

using namespace std;

// Used later in handleRequest when extracting the file name from url
string getFileName(const string request) {
	// Need a C string to tokenize, use a vector and cast
	vector<char> vRequest(request.size() + 1);
	memcpy(&vRequest.front(), request.c_str(), request.size() + 1);
	char* cRequest = vRequest.data();
	
	// Strip method
	char* token = strtok(cRequest, " ");

	// Now we have the url
	token = strtok(NULL, " ");

	string fileName = "";

	if (token != NULL) fileName = token;

	if (fileName == "/")
		fileName = "index.html";
	else if(fileName[0] == '/')
		fileName = fileName.substr(1, string::npos);

	return fileName;
}

Server::Server(int port, string root) : portNumber(port), docRoot(root) {
    // Root should always end in trailing slash
    if (docRoot[docRoot.length() - 1] != '/')
        docRoot += "/";

    cout << "Starting server with document root: ";

    // Relative path, print current working directory
    if (docRoot[0] != '/') {
        char buf[PATH_MAX];
        if(getcwd(buf, sizeof(buf)))
            cout << buf << "/";
    }
	
	// Print the rest of the path
    cout << docRoot << endl;
}

Server::~Server() {
	freeChildren(SIGINT);
}

// Kill all the children
void Server::freeChildren(int signal) {
	set<pid_t>::iterator i; // iterator through childForks
	// Kill them all
	for(i = childForks.begin(); i != childForks.end(); i++)
        kill(*i, signal);

	// Wait until all of them are killed to clear the set
    waitpid(-1, NULL, 0);
    childForks.clear();
}

// Free the memory of a dead child
void Server::freeChild(pid_t pid) {
	// Get the index of the dead child
    set<pid_t>::iterator i = childForks.find(pid);

	// If the child index still exists, free the memory
    if(i != childForks.end()) childForks.erase(i);
}

void Server::listen() {
	socket.create();
	socket.bind(portNumber);

	// Check the port
	int socketPort = socket.portNumber();
	if (socketPort <= 0) {
		// Failed to open socket
		cout << "Failed to open a socket on port " << portNumber << ", aborting\n";
		exit(EXIT_FAILURE);
	} else if (socketPort != portNumber) {
		cout << "Warning: Unable to bind to " << portNumber << endl;
	}

	cout << "Listening on port: " << socketPort << endl;
	socket.listen(MAX_CONNECTIONS);

	while (true) {
		Socket newSocket;
		socket.accept(newSocket);

		pid_t pid;

		// Fork until success
		do {
			pid = fork();
			if(pid > 0) {
				childForks.insert(pid);
				break;
			} else {
				// Don't need the set for the child
				childForks.clear();

				// Handle the data, then exit when done
				string data;
				newSocket.receive(data);
				handleRequest(newSocket,data);
				exit(0);
			}
		} while(pid == 0);
	}
}

void Server::handleRequest(Socket& responseSocket, const string& request) {
	string fileName = getFileName(request);
	string fileExtension = "";
	string mimeType = "";
	string responseCode = "";

	stringstream header;
	ifstream ifs;
	string response;

	// Get the file extension (if it exists)
	size_t extensionIndex = fileName.rfind('.');
	if (string::npos != extensionIndex)
		fileExtension = fileName.substr(extensionIndex, string::npos);

	// Set the appropriate mime type
	transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
	if (fileExtension == ".html")
		mimeType = "text/html; charset=utf-8";
	else if (fileExtension == ".gif")
		mimeType = "image/gif";
	else if(fileExtension == ".jpeg" || fileExtension == ".jpg")
		mimeType = "image/jpeg";
	else if(fileExtension == ".bmp")
		mimeType = "image/bmp";
	else if(fileExtension == ".pdf")
		mimeType = "application/pdf";
	else
		mimeType = "text/plain";

	// Open the file
	if (fileName.size() > 0) {
		fileName = docRoot + fileName;
		ifs.open(fileName.c_str(), ifstream::in);
	}

	responseCode = ifs.is_open() ? HTTP_OK : HTTP_NOT_FOUND;

	char c = ifs.get();
	while (ifs.good()) {
		response.push_back(c);
		c = ifs.get();
	}
	ifs.close();

	// Build the response header
	header << "HTTP/1.1 " << responseCode << endl;

	// Echo output to console
	if (responseCode != HTTP_NOT_FOUND)
		header << "Content-Type: " << mimeType << endl;
	header << "Content-Length: " << responseCode.size() << endl << endl;
	responseSocket.send(header.str());
	responseSocket.send(responseCode);
	cout << "Client Request:" << endl << request << endl;
}

