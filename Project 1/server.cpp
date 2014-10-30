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
#include <ctime>
#include "sys/stat.h"
#include "unistd.h"
#include "time.h"

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
	bool duplicate = false;

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
	
	// we do not need this while loop, bool a variable make this while loop only loop once and it solves the problem of multiple favicon issue.
			
	
		
	while (true) {
		Socket newSocket;
		socket.accept(newSocket);
		
		string data = "";
		/*while (data == "") {
		
			if (newSocket.receive(data)) { 
			//cout << "data= " << data << endl;
			handleRequest(newSocket,data);
			}
		}*/
		pid_t pid;
		
		// Fork until success
		do {
			pid = fork();
			if(pid > 0) {
				childForks.insert(pid);
				waitpid(0, NULL, 0);
				break;
			} else {
				// Don't need the set for the child
				childForks.clear();
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

	// this delete favicon things
	if (fileName == "favicon.ico") {
		return;
	}
	
	//for last modifed date
	string path = docRoot + fileName;
	const char *cpath = path.c_str();

	struct tm* lastTime;
	struct stat attrib; // create a file attribute structure
	stat(cpath, &attrib);
	lastTime = gmtime(&(attrib.st_mtime));
	char modified[80];
	strftime (modified,80, "%a, %e %b %G %H:%M:%S GMT",lastTime); 
	////////

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

	// get date: information
	time_t rawtime; 
	struct tm * timeinfo; 
	char buffer [80];

	time ( &rawtime ); 
	timeinfo = localtime ( &rawtime );

	strftime (buffer,80, "%a, %e %b %G %H:%M:%S GMT",timeinfo); 
	/////////////////

	
	responseCode = ifs.is_open() ? HTTP_OK : HTTP_NOT_FOUND;
	
	char c = ifs.get();
    while ( ifs.good() ) {
        response.push_back(c);
        c = ifs.get();
    }
	ifs.close();
	
	// header build up

	//I dont know why but if I did not comment out following line of code does not write appropriately on client side. if I comment out the first header input which is HTTP...., it does send header appropriately but no new lines. 

	stringstream clientHeader;
	// Send the client header to the client
    clientHeader << "HTTP/1.1 " << responseCode << std::endl;
	if ( HTTP_NOT_FOUND == responseCode ) {
        clientHeader << "Content-Length: " << responseCode.size() << "\n\n";
        responseSocket.send( header.str().c_str() );
        responseSocket.send( responseCode.c_str() );
        return;
    }
    clientHeader << "Content-Type: " << mimeType << std::endl;
    clientHeader << "Content-Length: " << response.size() << "\n\n";

    responseSocket.send( clientHeader.str().c_str() );
    responseSocket.send( response.c_str() );
    
    // Console server response
    stringstream serverHeader;
    serverHeader <<  "HTTP/1.1 " << responseCode << endl;
	serverHeader <<  "Connection: keep-alive" << endl;
	serverHeader << "Date: " << buffer << endl;
	serverHeader << "Server: Apache/2.2.3 (CentOS)" << endl;
	serverHeader << "Last-Modified: " << modified << endl;
	serverHeader << "Content-Type: " << mimeType << endl;
	serverHeader << "Content-Length: " << response.size() << endl;
	
	cout << endl << "Client Request:" << endl << request;

	cout << "Server Response:" << endl;
	cout  << serverHeader.str() << endl;
}

