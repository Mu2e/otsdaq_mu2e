// ots_udp_sw_emulator.cpp
//    by rrivera at fnal dot gov
//	  created Feb 2016
//
// This is a simple emulator of a "data gen" front-end (hardware) interface
// using the otsdaq UDP protocol.
//
// Protocol is specified
// https://docs.google.com/document/d/1i3Z07n8Jq78NwgUFdjAv2sLGhH4rWjHeYEScAWBzSyw/edit?usp=sharing
//
// compile with:
// g++ ots_udp_sw_emulator.cpp -o sw.o
//
// if developing, consider appending -D_GLIBCXX_DEBUG to get more
// descriptive error messages
//
// run with:
//./sw.o localhost <type-of-test>
//   or
//./sw.o ip.of.hw.o <type-of-test>
//
// 1 is write and read test
// 2 is data stream test

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

using namespace std;

#define HWPORT "4950"  // the port of the front end (hardware) target
#define MAXBUFLEN 1492

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int makeSocket(char* ip, int port)
{
	int                     sockfd;
	struct addrinfo         hints, *servinfo, *p;
	int                     rv;
	int                     numbytes;
	struct sockaddr_storage their_addr;
	socklen_t               addr_len;
	char                    s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if((rv = getaddrinfo(ip, HWPORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("sw: socket");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "sw: failed to create socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	return sockfd;
}

int main(int argc, char* argv[])
{
	// 2 types of tests defined:
	//	1. write and read
	//	2. burst data read test

	int                     sockfd;
	struct addrinfo         hints, *servinfo, *p;
	int                     rv;
	int                     numbytes;
	struct sockaddr_storage their_addr;
	socklen_t               addr_len;
	char                    s[INET6_ADDRSTRLEN];

	if(argc != 3)
	{
		fprintf(stderr, "usage: sw hostname type-of-test\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if((rv = getaddrinfo(argv[1], HWPORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("sw: socket");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "sw: failed to create socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	//////////////////////////////////////////////////////////////////////
	////////////// ready to go //////////////
	//////////////////////////////////////////////////////////////////////

	char         buff[MAXBUFLEN];
	unsigned int packetSz;
	int          type = atoi(argv[2]);
	cout << "sw: Line " << __LINE__ << ":::"
	     << "Type of Test: " << type << endl;

	uint64_t val = 0;
	uint64_t addr;
	bool     a    = 4;
	string   test = "hehehel";
	if(a != test.size())
		cout << endl;

	const unsigned int RX_ADDR_OFFSET = 2;
	const unsigned int RX_DATA_OFFSET = 10;
	const unsigned int TX_DATA_OFFSET = 2;

	switch(type)
	{
	case 1:
		// write and read

		// setup write packet
		// ///////////////////////////////////////////////////////////
		buff[0] = 1;       // write
		buff[1] = 1;       // num of quadwords
		addr    = 0x1001;  // data_gen_cnt
		memcpy((void*)&buff[RX_ADDR_OFFSET], (void*)&addr, 8);

		val = 4;
		for(int i = 0; i < buff[1]; ++i, ++val)
			memcpy((void*)&buff[RX_DATA_OFFSET + i * 8],
			       (void*)&val,
			       8);  // increment each time
		packetSz = RX_DATA_OFFSET + buff[1] * 8;

		if((numbytes = sendto(sockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
		   -1)
		{
			perror("sw: sendto");
			exit(1);
		}
		printf("sw: sent %d bytes to %s\n", numbytes, argv[1]);

		buff[0] = 0;  // read request
		              // ///////////////////////////////////////////////////////////
		packetSz = RX_DATA_OFFSET;

		if((numbytes = sendto(sockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
		   -1)
		{
			perror("sw: sendto");
			exit(1);
		}
		printf("sw: sent %d bytes to %s\n", numbytes, argv[1]);
		printf("sent packet contents: ");

		for(int i = 0; i < numbytes; ++i)
		{
			printf("%2.2X", (unsigned char)buff[i]);
			if(i % 8 == 7)
				printf("\n");
		}
		printf("\n");

		// read response ///////////////////////////////////////////////////////////
		if((numbytes = recvfrom(sockfd,
		                        buff,
		                        MAXBUFLEN - 1,
		                        0,
		                        (struct sockaddr*)&their_addr,
		                        &addr_len)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}

		printf("sw: got read response from %s\n",
		       inet_ntop(their_addr.ss_family,
		                 get_in_addr((struct sockaddr*)&their_addr),
		                 s,
		                 sizeof s));
		printf("sw: packet is %d bytes long\n", numbytes);
		printf("recv packet contents: ");

		for(int i = 0; i < numbytes; ++i)
		{
			printf("%2.2X", (unsigned char)buff[i]);
			if(i % 8 == 7)
				printf("\n");
		}
		printf("\n");

		memcpy((void*)&val, (void*)&buff[TX_DATA_OFFSET], 8);
		cout << "sw: Line " << __LINE__ << ":::"
		     << "Value read: " << hex << val << endl;

		break;
	case 2:
		// burst data read test

		// setup write packet
		// ///////////////////////////////////////////////////////////
		buff[0] = 1;                   // write
		buff[1] = 1;                   // num of quadwords
		addr    = 0x0000000100000009;  // data enable
		memcpy((void*)&buff[RX_ADDR_OFFSET], (void*)&addr, 8);
		memset((void*)&buff[RX_DATA_OFFSET], 0, 7);
		memset((void*)&buff[RX_DATA_OFFSET + 7], 1, 1);  // enable data
		packetSz = RX_DATA_OFFSET + buff[1] * 8;

		if((numbytes = sendto(sockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
		   -1)
		{
			perror("sw: sendto");
			exit(1);
		}
		printf("sw: sent %d bytes to %s\n", numbytes, argv[1]);

		// setup write packet
		// ///////////////////////////////////////////////////////////
		buff[0] = 1;       // write
		buff[1] = 1;       // num of quadwords
		addr    = 0x1001;  // data_gen_cnt
		memcpy((void*)&buff[RX_ADDR_OFFSET], (void*)&addr, 8);

		val = 4;  // this will be data count value
		memcpy((void*)&buff[RX_DATA_OFFSET], (void*)&val, 8);
		packetSz = RX_DATA_OFFSET + buff[1] * 8;

		if((numbytes = sendto(sockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
		   -1)
		{
			perror("sw: sendto");
			exit(1);
		}
		printf("sw: sent %d bytes to %s\n", numbytes, argv[1]);

		// read data packets
		// ///////////////////////////////////////////////////////////
		{
			int sz = val;  // get from read in TYPE 1
			cout << "sw: Line " << __LINE__ << ":::"
			     << "Number of packets expecting: " << sz << endl;
			for(int i = 0; i < sz; ++i)
			{
				cout << "sw: Line " << __LINE__ << ":::"
				     << "Waiting for data packet: " << i << endl;

				// read response
				// ///////////////////////////////////////////////////////////
				if((numbytes = recvfrom(sockfd,
				                        buff,
				                        MAXBUFLEN - 1,
				                        0,
				                        (struct sockaddr*)&their_addr,
				                        &addr_len)) == -1)
				{
					perror("recvfrom");
					exit(1);
				}

				printf("sw: got read response from %s\n",
				       inet_ntop(their_addr.ss_family,
				                 get_in_addr((struct sockaddr*)&their_addr),
				                 s,
				                 sizeof s));
				printf("sw: packet is %d bytes long\n", numbytes);
				printf("recv packet contents: ");

				for(int i = 0; i < numbytes; ++i)
				{
					printf("%2.2X", (unsigned char)buff[i]);
					if(i % 8 == 7)
						printf("\n");
				}
				printf("\n");

				memcpy((void*)&val, (void*)&buff[TX_DATA_OFFSET], 8);
				cout << "sw: Line " << __LINE__ << ":::"
				     << "Value read: " << val << endl;
			}
		}

		// setup write packet
		// ///////////////////////////////////////////////////////////
		buff[0] = 1;                   // write
		buff[1] = 1;                   // num of quadwords
		addr    = 0x0000000100000009;  // data enable
		memcpy((void*)&buff[RX_ADDR_OFFSET], (void*)&addr, 8);
		memset((void*)&buff[RX_DATA_OFFSET], 0, 7);
		memset((void*)&buff[RX_DATA_OFFSET + 7], 0, 1);  // disable data
		packetSz = RX_DATA_OFFSET + buff[1] * 8;

		if((numbytes = sendto(sockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
		   -1)
		{
			perror("sw: sendto");
			exit(1);
		}
		printf("sw: sent %d bytes to %s\n", numbytes, argv[1]);

		// read response ///////////////////////////////////////////////////////////
		if((numbytes = recvfrom(sockfd,
		                        buff,
		                        MAXBUFLEN - 1,
		                        0,
		                        (struct sockaddr*)&their_addr,
		                        &addr_len)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}

		printf("sw: got read response from %s\n",
		       inet_ntop(their_addr.ss_family,
		                 get_in_addr((struct sockaddr*)&their_addr),
		                 s,
		                 sizeof s));
		printf("sw: packet is %d bytes long\n", numbytes);
		printf("recv packet contents: ");

		for(int i = 0; i < numbytes; ++i)
		{
			printf("%2.2X", (unsigned char)buff[i]);
			if(i % 8 == 7)
				printf("\n");
		}
		printf("\n");
		break;
	default:
		cout << "sw: Line " << __LINE__ << ":::"
		     << "INVALID Type of Test: " << type << endl;
	}

	close(sockfd);

	return 0;
}
