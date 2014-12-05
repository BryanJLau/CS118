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
    cout << "./server -c X - \% of packets to pretend to corrupt (int).\n";
    cout << "./server -d X - \% of packets to pretend to drop (int).\n";
    cout << "./server -w X - Number of windows to use (int).\n";
    return;
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
    int port, corruptRate, dropRate, windows;
    string fileName, optString;
    port = corruptRate = dropRate= windows = -1;
    fileName = "";
    
    char c, ch;
    
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(argc == 1) {
        help();
        return 0;
    }

    while ((c = getopt (argc, argv, "p:c:d:w:")) != -1) {
	    switch (c) {
		    case 'p':
			    // Specify port number
			    optString.assign(optarg);
			    cout << "Setting port to " + optString << endl;
			    port = atoi(optarg);
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
			    cout << "Setting drop rate to " << optString <<
			        "\%" << endl;
			    dropRate = atoi(optarg);
			    break;
		    case 'w':
			    // Specify window count
			    optString.assign(optarg);
			    cout << "Setting window count to " << optString << endl;
			    windows = atoi(optarg);
			    break;
		    case '?':
			    ch = (char) optopt;
			    cerr << "Option -" << ch << " requires an argument.\n";
			    if (isprint (optopt)) {
				    cerr << "Unknown option `" << ch << "'.\n";
					help();
			    } else
				    cerr << "Unknown option character `\\x" << ch << "'.\n";
				    return 1;
			    break;
		    default:
			    abort();
	    }
    }

    if(port == -1) {
        // Empty port, set to default
        cout << "No port specified, using default port: 12345.\n";
        port = 12345;
    }
    if(corruptRate < 0) {
        // Negative corruptRate
        cout << "No corrupt rate specified, using default rate: 0\%.\n";
        corruptRate = 0;
    }
    if(dropRate < 0) {
        // Negative dropRate
        cout << "No drop rate specified, using default rate: 0\%.\n";
        dropRate = 0;
    }
    if(windows < 0) {
        // Negative windows
        cout << "No window count specified, using default : 4.\n";
        windows = 4;
    }

    connection = new GbnProtocol(corruptRate, dropRate, false, windows);
    if(!connection->listen(port)) {
    	// Listen failed
    	cout << "Server failed to listen.\n";
    	delete connection;
    	connection = NULL;
    	exit(-1);
    }
    
    cout << "Server is now listening on port " << 
        connection->portNumber() << endl;
	
	while(true) {
		// Wait for an actual connection
		if(!connection->accept()) continue;
		
		int file = open(connection->fileName.c_str(), O_RDONLY);

		if(file == -1) {
		    cout << "Invalid file request: \"" << connection->fileName << "\"\n";
		    connection->close();
		    continue; // We want to keep the server alive, not destroy it
		} else {
		    cout << "Preparing to send file...\n";
			string data = "";
			char buffer[MAX_BUFFER + 1]; // Need a \0 at the end
			memset(buffer, 0, MAX_BUFFER);
	
			// Read the contents of the file into a string
			int length = 0;
		
			while((length = read(file, buffer, MAX_BUFFER)) > 0) {
				data.append(buffer, length);
				memset(buffer, 0, MAX_BUFFER);
			}
			// Send the data and close
			connection->sendData(data);
			connection->close();
		}
	}
	
	return 0;
}

