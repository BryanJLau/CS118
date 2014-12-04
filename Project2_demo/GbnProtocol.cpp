#include "GbnProtocol.h"
#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

GbnProtocol::GbnProtocol(int cRate, int dRate) {
    corruptRate = cRate;
    dropRate = dRate;
    sockFd = -1;
    finned = listening = connected = false;
    
    memset(&remote, 0, sizeof(remote));
    memset(&local, 0, sizeof(remote));
    srand(time(0));
}

void GbnProtocol::dropPacket(packet &pkt) {
	// To simulate a drop, we'll simply simulate an invalid packet
	memset(&pkt, 0, sizeof(pkt));
}

int GbnProtocol::portNumber() {
	sockaddr_in addr;
	socklen_t length = sizeof(addr);
	if(getsockname(sockFd, (sockaddr*) &addr, &length) == -1)
		// Failed to get the port number
		return -1;
	
	return ntohs(addr.sin_port);
}

bool GbnProtocol::connect(string const &address, int const port, bool ack) {
    cout << "Attempting connection to: " << address << ":" << port << endl;
        
    if(!listening)
        if(!bind(port)) {
            cout << "Failed to bind socket.\n";
            return false;
        }
    
    finned = false;
    
    // Set remote info
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    if(inet_pton(AF_INET, address.c_str(), 
        (void*) &remote.sin_addr.s_addr) != 1) {
        close();
        return false;
    }
    
    // Socket timeout
    timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;
    if(setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout,
        sizeof(timeout)) < 0) {
        cout << "Failed to set socket timeout.\n";
        close();
        return false;
    }
    
    // Send SYN
    packet outPacket = buildPacket();
    setSyn(outPacket);
    // If this is an ACK as well as a SYN...
    if(ack) setSynAck(outPacket);
    
    if(!sendPacket(outPacket)) {
        cout << "Failed to send SYN packet.\n";
        close();
        return false;
    }
    
    packet inPacket;
    timeval start, now;
    gettimeofday(&start, NULL);
    int deltaSec = TIMEOUT_SEC;
    int deltaUsec = TIMEOUT_USEC;
    
    do {
        // Attempt to receive the SYNACK
        if(receivePacket(inPacket) && isSynAck(inPacket)) {
            cout << "Connection successful to: " << address << ":" 
            	<< port << endl;
            return true;
        } else if (errno == EWOULDBLOCK) {
            // Simulated socket timeout
            break;
        }
        
        // Advance time
        gettimeofday(&now, NULL);
    } while(now.tv_usec - start.tv_usec < deltaUsec &&
            now.tv_sec - start.tv_sec < deltaSec);
    
    // Time out
    close();
    cout << "Connection to: " << address << ":" << port <<
     	" timed out." << endl;
}

bool GbnProtocol::bind(const int port) {
    close();
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);


    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd == -1) {
        close(true);
        return false;
    }

    if (::bind(sockFd, (struct sockaddr*) &local, sizeof(local)) == 0) {

        return true;
    } else {
        cout << "bind failure to " << port <<endl;
        return false;
    }

}

void GbnProtocol::close(bool destroy) {
    if((!listening && sockFd != -1) || (listening && connected)) {
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remote.sin_addr.s_addr, addr, sizeof(addr));
        cout << "Closing connection to " << addr << ":" <<
            ntohs(remote.sin_port);
        
        // Initialize info
        int timeouts = 0;
        bool acked = false;
        bool resend = true;
        packet outPacket, inPacket;
        
        // Wait for FINACK
        while(!acked && timeouts < MAX_HANDSHAKES) {
            if(resend) {
                outPacket = buildPacket();
                setFin(outPacket);
                sendPacket(outPacket);
                resend = false; // Assume it works
            }
            
            if(receivePacket(inPacket)) {
                acked = isFinAck(inPacket);
                if(!finned) finned = isFin(inPacket);
            } else if(errno == EWOULDBLOCK) {
                // Simulated drop
                resend = true;
                timeouts++;
            }
        }
        
        if(acked && !finned) {
            // FINACK'ed, but not finned
            timeouts = 0;
            do {
                receivePacket(inPacket);
                finned = isFin(inPacket);
                if (errno == EWOULDBLOCK) timeouts++;
            } while (!finned && timeouts < MAX_HANDSHAKES);
        }
        if(finned) cout << "Connection successfully closed.\n";
        else {
            // Timeout from above, close it anyway
            finned = true;
            cout << "FIN timeout, closing connection anyway.\n";
        }
    }
    
    // Zero out the remote information
    memset(&remote, 0, sizeof(remote));
    
    // Close sockets if destroy is true or if there's no listener anymore
    if(destroy || !listening) {
        listening = false; // Stop listening if destroy because no socket
        if(sockFd != -1) ::close(sockFd);
        sockFd = -1;
        memset(&local, 0, sizeof(local));
    }
    
    // We want to still allow incoming connections
    if(!destroy && listening) {
        // To do this, we block the socket timeout
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout,
            sizeof(timeout));
        connected = false;
    }
}

bool GbnProtocol::listen(int const port) {
    close();
    listening = bind(port);
    return listening;
}

bool GbnProtocol::accept() {
	char addr[INET_ADDRSTRLEN];
	sockaddr_in inAddr;
	
	// You can't accept if you're not listening!
	if(!listening) return false;
	
	// Disconnect, if it's still connected from an intact close
	connected = false;
	
	// Try to connect
	while(!connected) {
		memset(&inAddr, 0, sizeof(inAddr));
		packet inPacket;
		// If you can't receive a packet, keep looping
		if(!receivePacket(inPacket, &inAddr)) continue;
		
		// Received a packet, see if it's a SYN packet since we're accepting
		if(isSyn(inPacket)) {
			inet_ntop(AF_INET, &inAddr.sin_addr.s_addr, addr, sizeof(addr));
			// SYNACK the SYN we just received
			connected = connect(addr, inPacket.header.sourcePort, true);
			cout << "Connecting to " << addr << ":" <<
				inPacket.header.sourcePort << endl;
		} else {
			dropPacket(inPacket);
			cout << "Received a non-SYN packet, dropping it.\n";
		}
	}
	
	return connected;
}

bool GbnProtocol::sendData(string const &data) {
	return false;
}

bool GbnProtocol::sendPacket(packet const &pkt) {
	size_t actualLength = 
		min(sizeof(gbnHeader) + pkt.header.length, sizeof(packet));
	size_t transmittedLength =
		sendto(sockFd, &pkt, actualLength, 0, (struct sockaddr*) &remote,
			sizeof(remote));
	return actualLength == transmittedLength;
}

int GbnProtocol::receiveData(char *data) {


    cout << "receivedata func" << endl;

    struct sockaddr_in rec_addr;
    unsigned char buf[2048];
    // struct sockaddr_in receiver_add;
    socklen_t addrlen = sizeof(rec_addr);
    int recvlen = recvfrom(sockFd, buf, sizeof(buf)-1, 0, 
        (struct sockaddr *)&rec_addr, &addrlen);
    memcpy(data, buf, recvlen);
    data[recvlen] = '\0';
    
    if (recvlen > 0) {
        cout << "recvlen = " << recvlen << endl;
        return recvlen;
    } else  {
        cout << "fail recvfrom" << endl;
        return -1;
    }
}

bool GbnProtocol::receivePacket(packet &pkt, sockaddr_in *sockaddr) {
	return false;
}

GbnProtocol::packet GbnProtocol::buildPacket(string const &data,
	size_t maxSize, size_t offset) {
	packet retPacket;
	memset(&retPacket, 0, sizeof(retPacket));
	
	// Make sure offset is valid, return empty packet
	if(offset > data.size()) return retPacket;
	
	unsigned int dataSize = (unsigned int) min(
		sizeof(retPacket.payload), data.size() - offset);
	
	// Initialize the header with default values
	retPacket.header.sourcePort = ntohs(local.sin_port);
	retPacket.header.destinationPort = ntohs(remote.sin_port);
	retPacket.header.sequenceNumber = 0;
	retPacket.header.ackNumber = 0;
	retPacket.header.length = max((unsigned int) 0, dataSize);
	retPacket.header.flags = 0;
	
	memcpy(&retPacket.payload, (data.substr(offset)).c_str(),
		retPacket.header.length);
	return retPacket;
}

