// This is a simple emulator of a "data gen" front-end (hardware) interface
// using the otsdaq UDP protocol.
//
// compile with:
// g++ udp_data_emulator.cpp -o test.o
//
// if developing, consider appending -D_GLIBCXX_DEBUG to get more
// descriptive error messages
//
// run with:
//./hw.o
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

//#define ZED_IP   "192.168.133.6"    // the ZED IP users will be connecting to
//#define ZED_IP             "192.168.133.5"    // the ZED IP users will be
// connecting to
#define COMMUNICATION_PORT "2035"  // the port on ZedBoard for communicating with XDAQ
#define STREAMING_PORT "2036"      // the port on ZedBoard for streaming to XDAQ
#define DESTINATION_IP "192.168.133.5"  // the IP for the destination of the datastream
#define DESTINATION_PORT 2039           // the port for the destination of the datastream
#define MAXBUFLEN 1492

//========================================================================================================================
// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//========================================================================================================================
int makeSocket(const char* ip, const char* port, struct addrinfo*& addressInfo)
{
	int                     socketId = 0;
	struct addrinfo         hints, *servinfo, *p;
	int                     sendSockfd = 0;
	int                     rv;
	int                     numbytes;
	struct sockaddr_storage their_addr;
	char                    buff[MAXBUFLEN];
	socklen_t               addr_len;
	char                    s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	//    hints.ai_family   = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_family   = AF_INET;  // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	if(ip == NULL)
		hints.ai_flags = AI_PASSIVE;  // use my IP

	if((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((socketId = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("listener: socket");
			continue;
		}

		if(bind(socketId, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socketId);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);
	return socketId;
}

//========================================================================================================================
struct sockaddr_in setupSocketAddress(const char* ip, unsigned int port)
{
	// std::cout << __PRETTY_FUNCTION__ << std::endl;
	// network stuff
	struct sockaddr_in socketAddress;
	socketAddress.sin_family = AF_INET;      // use IPv4 host byte order
	socketAddress.sin_port   = htons(port);  // short, network byte order

	if(inet_aton(ip, &socketAddress.sin_addr) == 0)
	{
		std::cout << "FATAL: Invalid IP address " << ip << std::endl;
		exit(0);
	}

	memset(&(socketAddress.sin_zero), '\0', 8);  // zero the rest of the struct
	return socketAddress;
}

//========================================================================================================================
int send(int toSocket, struct sockaddr_in& toAddress, const std::string& buffer)
{
	std::cout << "Socket Descriptor #: " << toSocket << " ip: " << std::hex
	          << toAddress.sin_addr.s_addr << std::dec
	          << " port: " << ntohs(toAddress.sin_port) << std::endl;
	if(sendto(toSocket,
	          buffer.c_str(),
	          buffer.size(),
	          0,
	          (struct sockaddr*)&(toAddress),
	          sizeof(sockaddr_in)) < (int)(buffer.size()))
	{
		std::cout << "Error writing buffer" << std::endl;
		return -1;
	}
	return 0;
}

//========================================================================================================================
int receive(int fromSocket, struct sockaddr_in& fromAddress, std::string& buffer)
{
	struct timeval tv;
	tv.tv_sec  = 1;
	tv.tv_usec = 0;         // set timeout period for select()
	fd_set fileDescriptor;  // setup set for select()
	FD_ZERO(&fileDescriptor);
	FD_SET(fromSocket, &fileDescriptor);
	select(fromSocket + 1, &fileDescriptor, 0, 0, &tv);

	if(FD_ISSET(fromSocket, &fileDescriptor))
	{
		std::string bufferS;
		// struct sockaddr_in fromAddress;
		socklen_t addressLength = sizeof(fromAddress);
		int       nOfBytes;
		buffer.resize(MAXBUFLEN);
		if((nOfBytes = recvfrom(fromSocket,
		                        &buffer[0],
		                        MAXBUFLEN,
		                        0,
		                        (struct sockaddr*)&fromAddress,
		                        &addressLength)) == -1)
			return -1;

		// Confirm you've received the message by returning message to sender
		send(fromSocket, fromAddress, buffer);
		buffer.resize(nOfBytes);
		//		char address[INET_ADDRSTRLEN];
		//		inet_ntop(AF_INET, &(fromAddress.sin_addr), address,
		// INET_ADDRSTRLEN); 		unsigned long  fromIPAddress =
		// fromAddress.sin_addr.s_addr; 		unsigned short fromPort      =
		// fromAddress.sin_port;
	}
	else
		return -1;

	return 0;
}

//========================================================================================================================
int main(void)
{
	/////////////////////
	// Bind UDP socket //
	/////////////////////

	// sendSockfd = makeSocket(string("localhost").c_str(),myport,p);

	struct addrinfo hints, *servinfo, *p;

	int                communicationSocket = makeSocket(0, COMMUNICATION_PORT, p);
	int                streamingSocket     = makeSocket(0, STREAMING_PORT, p);
	struct sockaddr_in streamingReceiver =
	    setupSocketAddress(DESTINATION_IP, DESTINATION_PORT);
	struct sockaddr_in messageSender;

	std::string communicationBuffer;

	//////////////////////////////////////////////////////////////////////
	////////////// ready to go //////////////
	//////////////////////////////////////////////////////////////////////

	cout << "Waiting for DAQ communication..." << endl;
	bool           triggered = false;
	const unsigned ndata     = 2 * 64;
	unsigned       count     = 0;
	while(1)
	{
		// cout << time(NULL)%60 << endl;
		if(receive(communicationSocket, messageSender, communicationBuffer) >= 0)
		{
			cout << "Message received!!!" << endl;
			cout << communicationBuffer << endl;

			if(communicationBuffer == "START")
			{
				triggered = true;
			}
			else if(communicationBuffer == "STOP")
			{
				cout << 4 * 66 * count << endl;
				triggered = false;
				count     = 0;
			}
		}

		if(triggered)
		{
			unsigned simdata[ndata + 5];
			simdata[0]   = 0xFFFFFFFF;
			simdata[1]   = 0xFFFFFFFF;
			simdata[2]   = 0xFFFFFFFF;
			simdata[3]   = 0xFFFFFFFF;
			simdata[4]   = 0xFFFFFFFF;
			unsigned i_0 = 0x21;
			for(unsigned i = 0; i < ndata; i++)
			{
				unsigned noise = rand() & 0x00FF00FF;
				simdata[i + 5] = 0x00FF00FF + noise;

				std::cout << std::hex << std::setw(8) << std::setfill('0') << simdata[i]
				          << std::endl;
			}

			//            sendto(streamingSocket, simdata, sizeof(simdata), 0,
			//                   (struct sockaddr *)&streamingReceiver,
			//                   sizeof(streamingReceiver));
			++count;
		}
		usleep(1000);
	}

	close(communicationSocket);

	return 0;
}
