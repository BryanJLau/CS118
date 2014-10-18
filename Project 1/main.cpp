#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>

#include "socket.h"
#include "server.h"

using namespace std;

// We make this a global variable because it's used later on in
// exitting when we exit due to a signal
Server* server;

// Simple console output functions
void credits() {
	cout << "Project 1 for CS118 (Lu), Fall 2014\n";
	cout << "Written by Joey Kim and Bryan Lau\n";
}

void help() {
	cout << "Server defaults:\n";
	cout << "Default port: 12345\n";
	cout << "Default root: /var/www/\n\n";
	cout << "Server parameters:\n";
	cout << "./server - Run the server on the default port and root\n";
	cout << "./server -p X - Run the server on port X.\n";
	cout << "./server -w X - Sets the document root of the web server to be X.\n";
	cout << "./server -c - Displays author information.\n";
	cout << "./server -h - Displays parameter help.\n";
}

// Signal handler for the program
void signalHandler(int signal) {
    cout << endl << "Received signal " << signal << ", exiting" << endl;

	// Free all connections
    server->freeChildren(signal);

	// Free server memory
    delete server;
    server = NULL;

    exit(signal);
}

int main(int argc, char **argv) {
	char c, ch;
	int port = 12345;
	string docRoot = "/var/www/";
	
	// Signal handler
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

	while ((c = getopt (argc, argv, "w:p:ch")) != -1) 
		switch (c) {
			case 'p':
				// Specify port number
				if(optarg[0] == 0) {
					// Sanity check
					// Empty port, set to default
					cout << "No port specified, using default port.\n";
				} else {
					string portNumber;
					portNumber.assign(optarg);
					cout << "Setting port to " + portNumber << endl;
					port = atoi(optarg);
				}
				break;
			case 'w':
				// Specify document root
				if(optarg[0] == 0) {
					// Sanity check
					// Empty root, set to default
					cout << "No root specified, using default root.\n";
				} else {
					docRoot.assign(optarg);
					cout << "Setting root to " + docRoot << endl;
				}
				break;
			case 'c':
				// Credits
				credits();
				exit(0);
				break;
			case 'h':
				// Help
				help();
				exit(0);
				break;
			case '?':
				ch = (char) optopt;
				if (optopt == 'p' || optopt == 'w')
					cerr << "Option -" << ch << " requires an argument.\n";
				else if (isprint (optopt))
					cerr << "Unknown option `" << ch << "'.\n";
				else
					cerr << "Unknown option character `\\x" << ch << "'.\n";
					return 1;
				break;
			default:
				abort();
		}
	
	// Set up the server and start listening
	server = new Server(port, docRoot);
	server->listen();
	
	exit(0);
}

