// ots_udp_hw_emulator.cpp
//    by rrivera at fnal dot gov
//	  created Feb 2016
//
// This is a simple emulator of a "data gen" front-end (hardware) interface
// using the otsdaq UDP protocol.
//
// compile with:
// g++ ots_udp_hw_emulator.cpp -o hw.o
//
// if developing, consider appending -D_GLIBCXX_DEBUG to get more
// descriptive error messages
//
// run with:
//./hw.o
//

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>

// take only file name
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// use this for normal printouts
#define __PRINTF__ printf
#define __COUT__ cout << __FILENAME__ << std::dec << " [" << __LINE__ << "]\t"

// and use this to suppress
//#define __PRINTF__ if(0) printf
//#define __COUT__  if(0) cout

using namespace std;

#define MAXBUFLEN 1492
#define EMULATOR_PORT "4950"  // Can be also passed as first argument

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if(sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int makeSocket(const char* ip, int port, struct addrinfo*& p)
{
	int                     sockfd;
	struct addrinfo         hints, *servinfo;
	int                     rv;
	int                     numberOfBytes;
	struct sockaddr_storage their_addr;
	socklen_t               addr_len;
	char                    s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	char portStr[10];
	sprintf(portStr, "%d", port);
	if((rv = getaddrinfo(ip, portStr, &hints, &servinfo)) != 0)
	{
		__PRINTF__("getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			__PRINTF__("sw: socket\n");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		__PRINTF__("sw: failed to create socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);

	return sockfd;
}

int main(int argc, char** argv)
{
	std::string emulatorPort(EMULATOR_PORT);
	if(argc == 2)
		emulatorPort = argv[1];

	__COUT__ << std::hex << ":::"
	         << "\n\nUsing emulatorPort=" << emulatorPort << "\n"
	         << std::endl;

	std::string    streamToIP;
	unsigned short streamToPort;

	int                     sockfd;
	int                     sendSockfd = 0;
	struct addrinfo         hints, *servinfo, *p;
	int                     rv;
	int                     numberOfBytes;
	struct sockaddr_storage their_addr;
	char                    buff[MAXBUFLEN];
	socklen_t               addr_len;
	char                    s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family   = AF_UNSPEC;  // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags    = AI_PASSIVE;  // use my IP

	if((rv = getaddrinfo(NULL, emulatorPort.c_str(), &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("listener: socket");
			continue;
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "listener: failed to bind socket\n");

		__COUT__ << "\n\nYou can try a different port by passing an argument.\n\n";

		return 2;
	}

	freeaddrinfo(servinfo);

	//////////////////////////////////////////////////////////////////////
	////////////// ready to go //////////////
	//////////////////////////////////////////////////////////////////////

	// print address space
	std::stringstream addressSpaceSS;
	addressSpaceSS << "Address space:\n";
	addressSpaceSS << "\t 0x0000000000001001 \t W/R \t 'Data count'\n";
	addressSpaceSS << "\t 0x0000000000001002 \t W/R \t 'Data rate'\n";
	addressSpaceSS << "\t 0x0000000000001003 \t W/R \t 'LEDs'\n";
	addressSpaceSS << "\t 0x0000000100000006 \t W \t 'Stream destination IP'\n";
	// addressSpaceSS << "\t 0x0000000100000007 \t W \t 'Destination MAC address
	// ignored'\n";
	addressSpaceSS << "\t 0x0000000100000008 \t W \t 'Stream destination port'\n";
	addressSpaceSS << "\t 0x0000000100000009 \t W/R \t 'Burst data enable'\n";

	__COUT__ << addressSpaceSS.str() << "\n\n";

	// hardware "registers"
	uint64_t data_gen_cnt  = 0;
	uint64_t data_gen_rate = 100;  // number of loops to wait
	uint8_t  led_register  = 0;
	uint8_t  dataEnabled   = 0;

	const unsigned int RX_ADDR_OFFSET = 2;
	const unsigned int RX_DATA_OFFSET = 10;
	const unsigned int TX_DATA_OFFSET = 2;

	bool          wasDataEnable = false;
	unsigned char sequence      = 0;
	unsigned int  packetSz;

	// for timeout/select
	struct timeval tv;
	fd_set         readfds, masterfds;
	tv.tv_sec  = 0;
	tv.tv_usec = 0;  // 500000; RAR moved timeout to sleep to free up processor
	FD_ZERO(&masterfds);
	FD_SET(sockfd, &masterfds);

	time_t count = 0;

	int handlerIndex;
	int totalNumberOfBytes;

	while(1)
	{
		usleep(3000);  // sleep 3ms to free up processor (downfall is less
		               // responsive, but likely not noticeable)

		readfds = masterfds;  // copy to reset timeout select
		select(sockfd + 1, &readfds, NULL, NULL, &tv);

		if(FD_ISSET(sockfd, &readfds))
		{
			// packet received
			__COUT__ << std::hex << ":::"
			         << "Packet Received!" << std::endl;

			addr_len = sizeof their_addr;
			if((totalNumberOfBytes = recvfrom(sockfd,
			                                  buff,
			                                  MAXBUFLEN - 1,
			                                  0,
			                                  (struct sockaddr*)&their_addr,
			                                  &addr_len)) == -1)
			{
				perror("recvfrom");
				exit(1);
			}

			__COUT__ << ":::"
			         << "hw: got packet from "
			         << inet_ntop(their_addr.ss_family,
			                      get_in_addr((struct sockaddr*)&their_addr),
			                      s,
			                      sizeof s)
			         << std::endl;
			__COUT__ << ":::"
			         << "hw: packet total is " << totalNumberOfBytes << " bytes long"
			         << std::endl;
			//			__COUT__ << ":::" << "hw: packet contents \n";
			//			for(int i=0; i<totalNumberOfBytes; i++)
			//			{
			//				if(i%8==0) __COUT__ << "\t" << i << " "
			//<< std::endl;
			//				__PRINTF__("%2.2X", (unsigned
			// char)buff[handlerIndex + i]);
			//				//__COUT__ << std::hex << std::setw(2)
			//<< (int)(unsigned char)buff[i] << std::dec;
			//			}

			// treat as stacked packets
			handlerIndex = 0;

			while(handlerIndex < totalNumberOfBytes &&
			      (numberOfBytes = (buff[handlerIndex + 0]
			                            ? 18
			                            : 10)) &&  // assume write is 18 and read is 10
			      handlerIndex + numberOfBytes <= totalNumberOfBytes)
			{
				__COUT__ << ":::"
				         << "hw: packet byte index " << handlerIndex << std::endl;
				__COUT__ << ":::"
				         << "hw: packet is " << numberOfBytes << " bytes long"
				         << std::endl;

				for(int i = 0; i < numberOfBytes; i++)
				{
					if((i - RX_ADDR_OFFSET) % 8 == 0)
						__PRINTF__("\n");
					__PRINTF__("%2.2X", (unsigned char)buff[handlerIndex + i]);
					//__COUT__ << std::hex << std::setw(2) << (int)(unsigned char)buff[i]
					//<< std::dec;
				}
				__PRINTF__("\n");

				// handle packet
				if(numberOfBytes == 10 &&  // size is valid (type, size, 8-byte address)
				   buff[handlerIndex + 0] == 0)  // read
				{
					uint64_t addr;
					memcpy((void*)&addr, (void*)&buff[handlerIndex + RX_ADDR_OFFSET], 8);

					__COUT__ << std::hex << ":::"
					         << "Read address: 0x" << hex << addr;
					__PRINTF__(" 0x%16.16lX \n", addr);

					// setup response packet based on address
					buff[handlerIndex + 0] = 0;  // read type
					buff[handlerIndex + 1] =
					    sequence++;  // 1-byte sequence id increments and wraps

					switch(addr)  // define address space
					{
					case 0x1001:
						memcpy((void*)&buff[handlerIndex + TX_DATA_OFFSET],
						       (void*)&data_gen_cnt,
						       8);
						__COUT__ << std::hex << ":::"
						         << "Read data count: " << data_gen_cnt << endl;
						break;
					case 0x1002:
						memcpy((void*)&buff[handlerIndex + TX_DATA_OFFSET],
						       (void*)&data_gen_rate,
						       8);
						__COUT__ << std::hex << ":::"
						         << "Read data rate: " << data_gen_rate << endl;
						break;
					case 0x1003:
						memset((void*)&buff[handlerIndex + TX_DATA_OFFSET + 1], 0, 7);
						memcpy((void*)&buff[handlerIndex + TX_DATA_OFFSET],
						       (void*)&led_register,
						       1);
						__COUT__ << std::hex << ":::"
						         << "Read LED register: " << (unsigned int)led_register
						         << endl;
						break;
					case 0x0000000100000009:
						memset((void*)&buff[handlerIndex + TX_DATA_OFFSET + 1], 0, 7);
						memcpy((void*)&buff[handlerIndex + TX_DATA_OFFSET],
						       (void*)&dataEnabled,
						       1);
						__COUT__ << std::hex << ":::"
						         << "Read data enable: " << dataEnabled << endl;
						break;
					default:
						__COUT__ << std::hex << ":::"
						         << "Unknown read address received." << endl;
					}

					packetSz = TX_DATA_OFFSET + 8;  // only response with 1 QW
					if((numberOfBytes = sendto(sockfd,
					                           buff,
					                           packetSz,
					                           0,
					                           (struct sockaddr*)&their_addr,
					                           sizeof(struct sockaddr_storage))) == -1)
					{
						perror("hw: sendto");
						exit(1);
					}
					__PRINTF__("hw: sent %d bytes back. sequence=%d\n",
					           numberOfBytes,
					           (unsigned char)buff[handlerIndex + 1]);
				}
				else if(numberOfBytes >= 18 &&
				        (numberOfBytes - 2) % 8 ==
				            0 &&  // size is valid (multiple of 8 data)
				        buff[handlerIndex + 0] == 1)  // write
				{
					uint64_t addr;
					memcpy((void*)&addr, (void*)&buff[handlerIndex + RX_ADDR_OFFSET], 8);
					__COUT__ << std::hex << ":::"
					         << "hw: Line " << std::dec << __LINE__ << ":::"
					         << "Write address: 0x" << std::hex << addr;
					__PRINTF__(" 0x%16.16lX \n", addr);

					switch(addr)  // define address space
					{
					case 0x1001:
						memcpy((void*)&data_gen_cnt,
						       (void*)&buff[handlerIndex + RX_DATA_OFFSET],
						       8);
						__COUT__ << std::hex << ":::"
						         << "Write data count: " << data_gen_cnt << endl;
						count = 0;  // reset count
						break;
					case 0x1002:
						memcpy((void*)&data_gen_rate,
						       (void*)&buff[handlerIndex + RX_DATA_OFFSET],
						       8);
						__COUT__ << std::hex << ":::"
						         << "Write data rate: " << data_gen_rate << endl;
						break;
					case 0x1003:
						memcpy((void*)&led_register,
						       (void*)&buff[handlerIndex + RX_DATA_OFFSET],
						       1);
						__COUT__ << std::hex << ":::"
						         << "Write LED register: " << (unsigned int)led_register
						         << endl;
						// show "LEDs"
						__COUT__ << "\n\n";
						for(int l = 0; l < 8; ++l)
							__COUT__ << ((led_register & (1 << (7 - l))) ? '*' : '-');
						__COUT__ << "\n\n";
						break;
					case 0x0000000100000006:
					{
						uint32_t           ip;
						struct sockaddr_in socketAddress;
						memcpy(
						    (void*)&ip, (void*)&buff[handlerIndex + RX_DATA_OFFSET], 4);
						ip = htonl(ip);
						memcpy((void*)&socketAddress.sin_addr, (void*)&ip, 4);
						streamToIP = inet_ntoa(socketAddress.sin_addr);
						__COUT__ << std::hex << ":::"
						         << "Stream destination IP: " << streamToIP << std::endl;
						__COUT__ << streamToIP << std::endl;
					}
					break;
					case 0x0000000100000007:
						__COUT__ << std::hex << ":::"
						         << "Destination MAC address ignored!" << std::endl;
						break;
					case 0x0000000100000008:
					{
						// unsigned int myport;
						memcpy((void*)&streamToPort,
						       (void*)&buff[handlerIndex + RX_DATA_OFFSET],
						       4);
						__COUT__ << std::hex << ":::"
						         << "Stream destination port: 0x" << streamToPort << dec
						         << " " << streamToPort << endl;

						close(sendSockfd);
						sendSockfd = 0;
						sendSockfd = makeSocket(streamToIP.c_str(), streamToPort, p);
						if(sendSockfd != -1)
						{
							__COUT__ << "************************************************"
							            "********"
							         << endl;
							__COUT__ << "************************************************"
							            "********"
							         << endl;
							__COUT__ << std::hex << ":::"
							         << "Streaming to ip: " << streamToIP << " port: 0x"
							         << streamToPort << endl;
							__COUT__ << "************************************************"
							            "********"
							         << endl;
							__COUT__ << "************************************************"
							            "********"
							         << endl;
						}
						else
							__COUT__ << std::hex << ":::"
							         << "Failed to create streaming socket to ip: "
							         << streamToIP << " port: 0x" << streamToPort << endl;
					}

					break;
					case 0x0000000100000009:
						memcpy((void*)&dataEnabled,
						       (void*)&buff[handlerIndex + RX_DATA_OFFSET],
						       1);
						__COUT__ << std::hex << ":::"
						         << "Write data enable: " << (int)dataEnabled << endl;
						count = 0;  // reset count
						break;
					default:
						__COUT__ << std::hex << ":::"
						         << "Unknown write address received." << endl;
					}
				}
				else
					__COUT__ << std::hex << ":::"
					         << "ERROR: The formatting of the string received is wrong! "
					            "Number of bytes: "
					         << numberOfBytes << " Read/Write " << buff[handlerIndex + 0]
					         << std::endl;

				handlerIndex += numberOfBytes;
			}

			__COUT__ << std::hex << ":::"
			         << "\n\n"
			         << addressSpaceSS.str() << "\n\n";

		}  // end packet received if
		else
		{
			// no packet received (timeout)

			//__COUT__ << "[" << __LINE__ << "]Is burst enabled? " << (int)dataEnabled
			//<< endl;
			if((int)dataEnabled)
			{
				// generate data
				//__COUT__ << "[" << __LINE__ << "]Count? " << count << " rate: " <<
				// data_gen_rate << " counter: " << data_gen_cnt << endl;
				if(count % data_gen_rate == 0 &&  // if delayed enough for proper rate
				   data_gen_cnt != 0)             // still packets to send
				{
					// if(count%0x100000 == 0)
					__COUT__ << std::hex << ":::"
					         << "Count: " << count << " rate: " << data_gen_rate
					         << " packet-counter: " << data_gen_cnt << std::endl;
					__COUT__ << std::hex << ":::"
					         << "Send Burst at count:" << count << std::endl;
					// send a packet
					buff[0] =
					    wasDataEnable ? 2 : 1;  // type := burst middle (2) or first (1)
					buff[1] = sequence++;       // 1-byte sequence id increments and wraps
					memcpy((void*)&buff[TX_DATA_OFFSET],
					       (void*)&count,
					       8);  // make data counter
					// memcpy((void *)&buff[TX_DATA_OFFSET],(void *)&data_gen_cnt,8);
					// //make data counter

					packetSz = TX_DATA_OFFSET + 8;  // only response with 1 QW
					// packetSz = TX_DATA_OFFSET + 180; //only response with 1 QW
					//	    			__COUT__ << packetSz << std::endl;
					//	    			std::string data(buff,packetSz);
					//	    			unsigned long long value;
					//	    			memcpy((void *)&value, (void *)
					// data.substr(2).data(),8); //make data counter
					//	    			__COUT__ << value << std::endl;

					if((numberOfBytes = sendto(
					        sendSockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
					   -1)
					{
						perror("Hw: sendto");
						exit(1);
					}
					__PRINTF__("hw: sent %d bytes back. sequence=%d\n",
					           numberOfBytes,
					           (unsigned char)buff[1]);

					if(data_gen_cnt != (uint64_t)-1)
						--data_gen_cnt;
				}

				wasDataEnable = true;
			}
			else if(wasDataEnable)  // send last in burst packet
			{
				wasDataEnable = false;
				__COUT__ << std::hex << ":::"
				         << "Send Last in Burst at count:" << count << endl;
				// send a packet
				buff[0] = 3;           // type := burst last (3)
				buff[1] = sequence++;  // 1-byte sequence id increments and wraps
				memcpy((void*)&buff[TX_DATA_OFFSET],
				       (void*)&count,
				       8);  // make data counter

				packetSz = TX_DATA_OFFSET + 8;  // only response with 1 QW

				if(sendSockfd != -1)
				{
					if((numberOfBytes = sendto(
					        sendSockfd, buff, packetSz, 0, p->ai_addr, p->ai_addrlen)) ==
					   -1)
					{
						perror("hw: sendto");
						exit(1);
					}
					__PRINTF__("hw: sent %d bytes back. sequence=%d\n",
					           numberOfBytes,
					           (unsigned char)buff[1]);
				}
				else
					__COUT__ << std::hex << ":::"
					         << "Send socket not defined." << endl;
			}

			++count;
		}
	}  // end main loop

	close(sockfd);

	return 0;
}
