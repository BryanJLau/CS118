#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include "GbnProtocol.h"

#define MAX_BUFFER 4096

using namespace std;

void help() {
    cout << "Project 2 for CS118 (Lu), Fall 2014\n";
    cout << "Written by Joey Kim and Bryan Lau\n";

    cout << "Server defaults:\n";
    cout << "Default port: 12345\n";
    cout << "Default file: README\n";
    cout << "Default corruption rate: 0\n";
    cout << "Default drop rate: 0\n\n";
    cout << "Server parameters:\n";
    cout << "./server - Display this message.\n";
    cout << "./server -p X - Run the server on port X.\n";
    cout << "./server -f X - Send file X.\n";
    cout << "./server -c X - \% of packets to pretend to corrupt (int).\n";
    cout << "./server -d X - \% of packets to pretend to drop (int).\n";
}

GbnProtocol *connection = NULL;

// Signal handler for the program
void signalHandler(int signal) {
    cout << endl << "Received signal " << signal << ", exiting" << endl;

    delete connection;
    connection = NULL;

    exit(signal);
}

int main(int argc, char **argv) {
    int port, corruptRate, dropRate;
    string fileName, optString;
    port = corruptRate = dropRate = -1;
    fileName = "";
    
    char c, ch;
    
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(argc == 1) {
        help();
        exit(0);
    }

    while ((c = getopt (argc, argv, "f:p:c:d:")) != -1) 
	    switch (c) {
		    case 'p':
			    // Specify port number
			    optString.assign(optarg);
			    cout << "Setting port to " + optString << endl;
			    port = atoi(optarg);
			    break;
		    case 'f':
			    // Specify document root
			    fileName.assign(optarg);
			    cout << "Setting file to " << fileName << endl;
			    break;
		    case 'c':
			    // Specify corrupt rate
			    optString.assign(optarg);
			    cout << "Setting corruption rate to " << optString <<
			        "\%" << endl;
			    corruptRate = atoi(optarg);
			    break;
		    case 'd':
			    // Specify drop rate
			    optString.assign(optarg);
			    cout << "Setting corruption rate to " << optString <<
			        "\%" << endl;
			    dropRate = atoi(optarg);
			    break;
		    case '?':
			    ch = (char) optopt;
			    cerr << "Option -" << ch << " requires an argument.\n";
			    if (isprint (optopt))
				    cerr << "Unknown option `" << ch << "'.\n";
			    else
				    cerr << "Unknown option character `\\x" << ch << "'.\n";
				    return 1;
			    break;
		    default:
			    abort();
	    }

    if(port == -1) {
        // Empty port, set to default
        cout << "No port specified, using default port: 12345.\n";
        port = 12345;
    }
    
    if(fileName == "") {
        // Empty root, set to default
        cout << "No file specified, using default file: README.\n";
        fileName = "README";
    }
    
    if(corruptRate == -1) {
        // Empty corruption rate, set to default
        cout << "No corruption rate specified, using default " <<
            "rate: 0\%.\n";
        corruptRate = 0;
    }
    
    if(dropRate == -1) {
        // Empty drop rate, set to default
        cout << "No drop rate specified, using default " << "rate: 0\%.\n";
        dropRate = 0;
    }
    
    connection = new GbnProtocol(corruptRate, dropRate);
    if(!connection->listen(port)) {
    	// Listen failed
    	cout << "Server failed to listen.\n";
    	delete connection;
    	connection = NULL;
    	exit(-1);
    }
    
    cout << "Server is now listening on port " << connection->portNumber()
    	<< endl;
	
	while(true) {
		// Wait for an actual connection
		if(!connection->accept()) continue;
		
		string recvData;
		connection->receiveData(recvData);
		
		// Open the file (if it exists)
		int fd = open(recvData.c_str(), O_RDONLY);
		// Failed to open the file
		if(fd == -1) {
			cout << "Invalid file request: \"" << recvData << "\"\n";
			connection->close();
			continue; // We want to keep the server alive, not destroy it
		}
		
		string data = "";
		char buffer[MAX_BUFFER + 1]; // Need a \0 at the end
		memset(buffer, 0, MAX_BUFFER);
		
		// Read the contents of the file into a string
		int length = read(fd, buffer, MAX_BUFFER);
		while(length > 0) {
			data.append(buffer, length);
			memset(buffer, 0, MAX_BUFFER);
		}
		
		// Send the data and close
		connection->sendData(data);
		connection->close();
	}
	
	return 0;
}

