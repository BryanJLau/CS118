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
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    }

    struct sockaddr_in rec_addr;

/* bind to an arbitrary return address */
/* because this is the client side, we don't care about the address */
/* since no application will initiate communication here - it will */
/* just send responses */
/* INADDR_ANY is the IP address and 0 is the socket */
/* htonl converts a long integer (e.g. address) to a network representation */
/* htons converts a short integer (e.g. port) to a network representation */

    memset((char *)&rec_addr, 0, sizeof(rec_addr));
    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(10000);
    inet_aton("127.0.0.1", &rec_addr.sin_addr);


    // if (bind(sockFd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0) {
    //     perror("bind failed");
    //     return 0;
    // }

    struct hostent *hp;
    string message = "";
    const void* c_message = message.c_str();



    cout << "Receiver open UDP connection on server's port " << port
        << endl;

    
    //memcpy((void *)&rec_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

    if (sendto(sockFd, c_message, sizeof(c_message), 0, 
        (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0) {
       perror("sendto failed");
        return 0;
    }
}

