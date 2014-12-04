#ifndef Gbn
#define Gbn

#include <netinet/in.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define WINDOW_SIZE 1024

#define MTU 1024
#define IP_HEADER 20
#define UDP_HEADER 8
#define MSS (MTU - IP_HEADER - UDP_HEADER)

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 100000
#define MAX_TRANSMITS 10
#define MAX_HANDSHAKES 5
#define MAX_DUP_ACK 3

/**
 *  We have 16 bits for the flag
 *  TCP uses Offset(4), Reserved (6), Control(6)
 *  Which are URG, ACK, PSH, RST, SYN, FIN
 *  Technically we can use any of the bits without offset or reserve
 *  We need ACK, SYN, and FIN at the very least
 *  We'll also use ACKS for the SYN and FIN 
 *  To signify the end of a file, we'll use an EOF, and an ACK for that as well
 *  To easily set the flags, we'll use masking on the flags part of the header
 */
#define COR_MASK    1 << 7;
#define EOFACK_MASK 1 << 6;
#define EOF_MASK    1 << 5;
#define SYNACK_MASK 1 << 4;
#define FINACK_MASK 1 << 3;
#define ACK_MASK    1 << 2;
#define SYN_MASK    1 << 1;
#define FIN_MASK    1 << 0;

using namespace std;

class GbnProtocol {
    public:
        GbnProtocol(int cRate, int dRate);
        
        bool connect(string const &address, int const port, bool ack);
        bool listen(int const port);
        bool accept();
        void close(bool destroy = false);
        
        bool sendData(string const &data);
        bool receiveData(string &data);
        int portNumber();
        
        struct gbnHeader {
            uint16_t sourcePort;
            uint16_t destinationPort;
            uint32_t sequenceNumber;
            uint32_t ackNumber;
            uint16_t length;
            uint16_t flags;
        };
        
        struct packet {
            gbnHeader header;
            char payload[MSS - sizeof(gbnHeader)];
        };
        
    private:
        bool listening;
        bool connected;
        bool finned;
        int sockFd;
        sockaddr_in remote;
        sockaddr_in local;
        
        int dropRate;
        int corruptRate;
        
        bool isCorrupted(packet &pkt) { return pkt.header.flags & COR_MASK; }
        bool isEofAck(packet &pkt) { return pkt.header.flags & EOFACK_MASK; }
        bool isEof(packet &pkt) { return pkt.header.flags & EOF_MASK; }
        bool isSynAck(packet &pkt) { return pkt.header.flags & SYNACK_MASK; }
        bool isFinAck(packet &pkt) { return pkt.header.flags & FINACK_MASK; }
        bool isAck(packet &pkt) { return pkt.header.flags & ACK_MASK; }
        bool isSyn(packet &pkt) { return pkt.header.flags & SYN_MASK; }
        bool isFin(packet &pkt) { return pkt.header.flags & FIN_MASK; }

        void setCorrupted(packet &pkt) { pkt.header.flags |= COR_MASK; }
        void setEofAck(packet &pkt) { pkt.header.flags |= EOFACK_MASK; }
        void setEof(packet &pkt) { pkt.header.flags |= EOF_MASK; }
        void setSynAck(packet &pkt) { pkt.header.flags |= SYNACK_MASK; }
        void setFinAck(packet &pkt) { pkt.header.flags |= FINACK_MASK; }
        void setAck(packet &pkt) { pkt.header.flags |= ACK_MASK; }
        void setSyn(packet &pkt) { pkt.header.flags |= SYN_MASK; }
        void setFin(packet &pkt) { pkt.header.flags |= FIN_MASK; }
        
        packet buildPacket(string const &data = "",
        	size_t maxSize = 0, size_t offset = 0);
        bool sendPacket(packet const &pkt);
        bool receivePacket(packet &pkt, sockaddr_in *sockaddr = NULL);
        void dropPacket(packet &pkt);
        
        bool bind(int port);
};

#endif

