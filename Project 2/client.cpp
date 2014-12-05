#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include "GbnProtocol.h"
// #include <sys/type.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAX_BUFFER 4096

using namespace std;

void help() {
    cout << "Project 2 for CS118 (Lu), Fall 2014\n";
    cout << "Written by Joey Kim and Bryan Lau\n";

    cout << "Receiver defaults:\n";
    cout << "./receiver -h X - Sender's hostname X.\n";
    cout << "./receiver -p X - Set UDP connection on port X.\n";
    cout << "./reciever -f X - Recever request file X.\n";
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

    int port;
    string fileName, optString, hostname;
    port = -1;
    fileName = "";
    
    char c, ch;
    
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(argc == 1) {
        help();
        exit(0);
    }

    while ((c = getopt (argc, argv, "h:f:p:")) != -1) 
        switch (c) {
            case 'h':
                optString.assign(optarg);
                cout << "Sender's hostname is " << optString <<endl;
                hostname = atoi(optarg);
                break;
            case 'p':
                // Specify port number
                optString.assign(optarg);
                cout << "connecting to " + optString << endl;
                port = atoi(optarg);
                break;
            case 'f':
                // Specify document root
                fileName.assign(optarg);
                cout << "Request file is " << fileName << endl;
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

    if(hostname == "") {
        cout << "No server's hostname , default hostname = localhost" <<endl;
        //default hostname = "localhost"
        hostname = "localhost";
    }
    
    char addr[INET_ADDRSTRLEN];
    memset(&addr, 0, sizeof(addr));
    
    struct hostent *he = gethostbyname(hostname.c_str());
    if(he && he->h_length > 0) {
        // Found the host!
        inet_ntop(AF_INET, he->h_addr_list[0], addr, sizeof(addr));
    } else {
        // Couldn't find host
        cout << "Could not find host: " << hostname << endl;
        exit(-1);
    }
    
    // Time to set up the connection and start sending
    // We're going to use 0 for the receiver since the sender has this
    connection = new GbnProtocol(0, 0, true);
    string fileData = "";
    if(!connection->connect(addr, port, false)) {
        cout << "Failed to connect.\n";
        exit(-1);
    } else {
        connection->sendData(fileName);
        connection->receiveData(fileData);
        connection->close();
    }
    
    cout << endl << fileData << endl;
    
    return 0;
}

