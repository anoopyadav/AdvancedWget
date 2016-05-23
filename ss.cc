// server.cc
// 
// Copyright 2013 anzy <anzy@ubuntu>
// 
// 
// 
#include <iostream>
#include <pthread.h>
#include "ss.h"
//using namespace std;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::to_string;
using std::thread;
using std::ofstream;
using std::ifstream;
using std::ios;

namespace
{
    char* get_basename(char *path)
    {
        char *base = strrchr(path, '/');
        return base ? base+1 : path;
    }
}

// Constructor
SteppingStone::SteppingStone()  {
	port = "51969";
	srand(time(NULL));
}

// Destructor
SteppingStone::~SteppingStone()  {
}

// Detect the machines IP address
int SteppingStone::getIpAddress()  {
	struct ifaddrs *ifAddrStruct;
    struct ifaddrs *ifa;
    void *tmpAddrPtr=NULL;
    //string ip = "0.0.0.0";

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(strcmp(ifa->ifa_name,"lo") != 0)  {
				cout << addressBuffer;
				freeifaddrs(ifAddrStruct);
				return 0;
				//return ip;
			} 
        } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            if(strcmp(ifa->ifa_name, "lo") != 0)  {
				cout << addressBuffer;
				freeifaddrs(ifAddrStruct);
				return 0;
			}
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return 1;
}

int SteppingStone::parseArguments(int argc, char** argv)  {
	if(argc == 1)  {
		return 0;
	}
	if(argc > 2)  {
		if(strcmp(argv[1], "-p") == 0)  {
			int i;
			for(i = 0; i < strlen(argv[2]); i++)  {
				if(!(argv[2][i] >= '0' && argv[2][i] <= '9'))  {
					cout << "Invalid port. Using default." << endl;
					return 1;
				}
			}
			
			// String contains only numbers, now check bounds
			int temp = atoi(argv[2]);
			if(temp > 0 && temp < 65536)  {
				// valid port number, assign to class member "port"
				port = argv[2];
				//sprintf(port, "%s", arg);
				//printf("Port: %d", temp);
			}
			else  {
				cout << "Invalid port. Using default." << endl;
				return 1;
			}
		}
	}
	else  {
		return 1;
	}
	return 1;
}
// Launch the Server
int SteppingStone::startServer()  {
	// Gethostname
	char name[128];
	gethostname(name, sizeof name);

	struct addrinfo hints;
	struct addrinfo *servinfo, *p; // will point to the results
	int status;
	int sockfd, new_fd; // File Descriptors
	int yes;
	struct sockaddr_storage their_addr; // Store the client's information
	
	char s[INET6_ADDRSTRLEN];
	//printf("Welcome to awget!\n");
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	
	// Call to addrinfo, get the connection information
	if ((status = getaddrinfo(NULL, "0", &hints, &servinfo)) != 0) {
		printf("getaddrinfo error:%s",gai_strerror(status));
		exit(1);
	}
	
	
	// Bind to the port
	for(p = servinfo; p != NULL; p = p->ai_next) {	
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			printf("server: socket\n");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			printf("setsockopt\n");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			printf("server: bind\n");
			continue;
		}
		break;
	}
	
	
	// Exit if bind fails
	if (p == NULL) {
		//printf("server: failed to bind\n");
		return 2;
	}
	
	
	// free the linked-list
	freeaddrinfo(servinfo); 
	
	
	// Start listening
	if (listen(sockfd, 10) == -1) {
		//printf("listen");
		exit(1);
	}           
	
	// Get Port number
	struct sockaddr_in localAddress;
	socklen_t *addressLength = (socklen_t*)malloc(sizeof(socklen_t));
	*addressLength = sizeof localAddress;
	getsockname(sockfd, (struct sockaddr*)&localAddress, addressLength);
	//cout << " at Port:" << (int)ntohs(localAddress.sin_port) << endl;
	int portNumber = (int)ntohs(localAddress.sin_port);
	port = to_string(portNumber);
	free(addressLength);
	
	// Main communication loop
	socklen_t *sin_size = (socklen_t*)malloc(sizeof(socklen_t));
	*sin_size = sizeof their_addr;
	printf("ss<%s", name);
	printf(",%d>:\n", (int)ntohs(localAddress.sin_port));

	while((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, sin_size)))  {
		if (new_fd == -1) {
			//printf("accept");
			return -1;
		}
		thread *t = new thread(&SteppingStone::handleConnection, this, new_fd);
		//t->detach();
		//inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		//printf("\nserver: got connection from %s", s);
		//printf("\nFound a friend. You recieve first.\n");
		//fflush(stdout);
	}
	return 0;
}

int SteppingStone::handleConnection(int sockfd)  {
	// Information for next stepping stone
	char ssHostname[16];
	char ssPort[6];
	vector<string> chainlist;
	//cout << "Thread dispatched!!" << endl;
	
	
	// Recieve Message	
	//while(1) { // this is the child process
	// Buffer variables
	//char *messageSize = new char[2];
	//char *buffer = new char[1024];
	char messageSize[2];
	char buffer[1024];
		int bytes;

		if ((bytes = recv(sockfd, messageSize, 2, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		
		// Extract the URL
		uint8_t msb = messageSize[0];
		uint8_t lsb = messageSize[1];
		uint16_t len =  (msb << 8) | lsb;
		//cout << len  << endl;
	
		
		//cout << buffer1 + 2 << endl;
		if ((bytes = recv(sockfd, buffer, len, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		//cout << buffer << endl;
		buffer[len] = '\0';
		url = buffer;
		
		// Display the request
		cout << "Request: " << url << endl;
		
		// Extract the chainlist
		if ((bytes = recv(sockfd, messageSize, 2, 0)) == -1) {
			printf("\nError recieving message!\n");
			exit(1);
		}
		len = 0;
		msb = messageSize[0];
		lsb = messageSize[1];
		len =  (msb << 8) | lsb;
		//cout << len  << endl;
	
		if(len !=0)  {
			if ((bytes = recv(sockfd, buffer, len, 0)) == -1) {
				printf("\nError recieving message!\n");
				exit(1);
			}
			// Null-terminate the string
			buffer[len] = '\0';
			//cout << "Debug:" << buffer	 << endl;
			
			char *pch = strtok (buffer," ");
			while(pch != NULL)  {
				chainlist.push_back(string(pch));
				pch = strtok (NULL, " ");
			}
			
		}
		//cout << chainlist.size() << endl;
		
		// Send to next ss
		if(chainlist.size() > 0)  {
			// Print Chainlist
			cout << "Chainlist is" << endl;
			for(int i = 0; i < chainlist.size(); i++)  {
				cout << "<" << chainlist[i] << ">" << endl;
			}
			//cout << "Boo Boo!" << endl;
			// Choose a stepping stone at random
			int index = rand() % chainlist.size();
			string target = chainlist[index];
			char *pch = strtok ((char*)target.c_str(),",");
			strcpy(ssHostname, pch);
			pch = strtok (NULL, " ");
			strcpy(ssPort, pch);
			//cout << hostname << " " << port << endl;
			
			// Remove the chosen ss
			vector<string>::iterator it = std::find(chainlist.begin(), chainlist.end(), chainlist[index]);
			//if (it != chainlist.end()) chainlist.erase(it);
			chainlist.erase(it);
			//cout << chainlist.size() << endl;
			
			// Print the chosen ss
			printf("Next ss is <%s,%s>\n", ssHostname, ssPort); 
			
			// Obtain new ssList
			string ssList = "";
			for(int i = 0; i < chainlist.size(); i++)  {
				ssList += chainlist[i];
				if(i < chainlist.size() - 1)  {
					ssList += " ";
				}
			}
			//cout << "Debug:" << ssList;
			
			// Prepare the data packet
			// Prepare the data packet
			uint16_t totalLength = ssList.length() + url.length();
			uint16_t length = url.length();
			buffer[0] = (uint8_t) ((length >> 8) & 0xff);
			buffer[1] = (uint8_t) (length & 0xff);
			strncpy(buffer + 2, url.c_str(), (size_t)(length));
			length = ssList.length();
			buffer[url.length() + 2] = (uint8_t) ((length >> 8) & 0xff);
			buffer[url.length() + 3] = (uint8_t) (length & 0xff);
			strncpy(buffer + url.length() + 4, ssList.c_str(), (size_t)(length));
			buffer[ssList.length() + url.length() + 4 + 1] = '\0';
			
			// connect to the next ss
			int numbytes, newfd;
			//char buffer[2];
			struct addrinfo hints, *serverInfo, *p;
			int rv;
			char ipAddress[INET6_ADDRSTRLEN];

			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_UNSPEC; //always the same. At least in this class
			hints.ai_socktype = SOCK_STREAM; //set tcp
			
			
			//cout << hostname << " " << port << endl;
			
			//get information about the host and port specified by the user
			if ((rv = getaddrinfo(ssHostname, ssPort, &hints, &serverInfo)) != 0) {
				printf("getaddrinfo: %s\n", gai_strerror(rv));
				exit(1);
				//return 1;
			}

			// loop through all the results and connect to the first we can
			for (p = serverInfo; p != NULL; p = p->ai_next) {
				if ((newfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
					perror("client: socket");
					continue;
				}

				if (connect(newfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(newfd);
					perror("client: connect");
					continue;
				}

				break;
			}

			if (p == NULL) {
				fprintf(stderr, "client: failed to connect\n");
				exit(1);
				//return 2;
			}

			freeaddrinfo(serverInfo); // all done with this structure
			//cout << buffer +2 << endl;
			// Send Request
			if (send(newfd, buffer, url.length() + ssList.length() + 5, 0) == -1)  { 
				printf("send\n");
				close(newfd);
				exit(0);
			}

			printf("Waiting for file...\n");
			
			// Get filename
			if ((numbytes = recv(newfd, messageSize, 2, 0)) == -1) {
				printf("\nError recieving message!\n");
				exit(1);
			}
			uint8_t msb = messageSize[0];
			uint8_t lsb = messageSize[1];
			uint16_t len =  (msb << 8) | lsb;
			//cout << "Filename length:" << len  << endl;
			
			
			//cout << buffer1 + 2 << endl;
			int bytes;
			//char *buf = new char[len + 1];
			char buf[512];
			if ((bytes = recv(newfd, buf, len, 0)) == -1) {
				printf("\nError recieving message!\n");
				exit(1);
			}
			buf[len] = '\0';
			string filename = buf;
			//delete[] buf;
			//cout << "Filename:" << filename << endl;
			
			// Timestamp
			long ts = time(NULL) + 5;
			string timestamp= to_string(ts);
			string tempFile = filename + "_" + timestamp;
			
			// Open a file to write
			ofstream out(tempFile, ios::binary);
			// Loop to read the file
			//while(recv(sockfd, buffer, 1024, 0) != -1)  {
			sleep(5);
			int counter = 0;
			while(1)  {
				//cout << "Hello" << endl;
				messageSize[0] = '\0';
				messageSize[1] = '\0';
				int rd = recv(newfd, messageSize, 2, 0);
				//cout << "Bytes read:" << rd;
				if(rd == 0)  {
					break;
				}
				if(rd < 0)  {
					cout << "Error reading file!!" << endl;
					exit(1);
				}
				// Read the length
				uint8_t msb = messageSize[0];
				uint8_t lsb = messageSize[1];
				uint16_t len = (msb << 8) | lsb;
				//cout << len << endl;
				rd = recv(newfd, buffer, len, 0);
				//cout << "Bytes read:" << rd << endl;
				out.write(buffer, len);
				if(len < 1024)  {
					close(newfd);
					break;
				}
	
				counter++;
				if(counter == 5)  {
					counter = 0;
					sleep(2);
				}
			}
			out.close();
			cout << "Received file \"" << filename << "\"" << endl;
			// Transmit the file
			// Open the downloaded file to transfer
			ifstream file(tempFile.c_str(), ios::binary|ios::ate);
			long size = file.tellg();
			file.seekg(0,ios::beg);
			
			// Transmit the file name
			totalLength = filename.length();
			buffer[0] = (uint8_t) ((totalLength >> 8) & 0xff);
			buffer[1] = (uint8_t) (totalLength & 0xff);
			strncpy(buffer + 2, filename.c_str(), (size_t)(totalLength));
			if (send(sockfd, buffer, totalLength + 2, 0) == -1)  { 
				printf("send\n");
				close(sockfd);
				exit(0);
			}
			
			// Do some calculations
			int iterations = size/1024;
			int residue = size%1024;
			// Transmit the file 1KB at a time
		
			//ofstream out("test.pdf", ios::binary);
			int count = 0;
			cout << "Transmitting file..." << endl;
			while(!file.eof())  {
				int index = 1024;
				if(count != iterations)  {
					file.read(buffer,1024);
					
					// Send the packet size
					messageSize[0] = (uint8_t) ((index >> 8) & 0xff);
					messageSize[1] = (uint8_t) (index & 0xff);
					if (send(sockfd, messageSize, 2, 0) == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					
					// Send to previous ss
					if (send(sockfd, buffer, index, 0) == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					count++;
				}
				else  {
					//cout << "Hello" << count << residue << endl;
					index = residue;
					//char *toSend = new char[index];
					file.read(buffer, index);
				
					// Send the packet size
					messageSize[0] = (uint8_t) ((index >> 8) & 0xff);
					messageSize[1] = (uint8_t) (index & 0xff);
					if (send(sockfd, messageSize, 2, 0) == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					
					// Send to previous ss
					if (send(sockfd, buffer, index, 0) == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					count++;
					break;
				}
				
				//cout << "Count:" << count << endl;
			
			
			}
			file.close();
			close(newfd);
			
			// Delete temp file
			string del = "rm " + tempFile;
			system(del.c_str());
			cout << "File Sent...Goodbye." << endl;
		}
		
		// Last ss, obtain the file
		else  {
			// Figure out the filename
			string filename;
            char * urlCstr = new char [url.length()+1];
            std::strcpy (urlCstr, url.c_str());
			string output = get_basename(urlCstr);
			//cout << "Output: " << output << endl;
			if(strlen(output.c_str()) == 0)  {
				filename = "index.html";
			}
			else {
				string address = url.substr(7, string::npos);
				if(output.compare(url) == 0)  {
					filename = "index.html";
				}
				else if(address.compare(output) == 0)  {
					filename = "index.html";
				}
				else  {
					filename = output;
				}
			}
			//cout << "Filename:" << filename  << endl;
			long ts = time(NULL);
			cout << "Issuing wget for file " << filename << endl;
			string timestamp= to_string(ts);
			string tempFile = filename + "_" + timestamp;
			string command = "wget -q -O " + tempFile +  " " + url;
			//char *cmd = new char[command.length() + 1];
			char cmd[512];
			strncpy(cmd, command.c_str(), command.length());
			cmd[command.length()] = '\0';
			//system("mkdir tmp");
			//cout << cmd << endl;
			system(cmd);
			//delete[] cmd;
			//printf("Done!\n");
			cout << "File downloaded!" << endl;
			
			// Open the downloaded file to transfer
			ifstream file(tempFile.c_str(), ios::binary|ios::ate);
			long size = file.tellg();
			//char *memblock = new char[size];
			file.seekg(0,ios::beg);
			//file.read(memblock,size);
			
			cout << "Relaying file... " << endl;
			// Transmit the file name
			uint16_t totalLength = filename.length();
			buffer[0] = (uint8_t) ((totalLength >> 8) & 0xff);
			buffer[1] = (uint8_t) (totalLength & 0xff);
			strncpy(buffer + 2, filename.c_str(), (size_t)(totalLength));
			if (send(sockfd, buffer, totalLength + 2, 0) == -1)  { 
				printf("send\n");
				close(sockfd);
				exit(0);
			}
			
			// Do some calculations
			int iterations = size/1024;
			int residue = size%1024;
			// Transmit the file 1KB at a time
			//char *chunk;
		
			//ofstream out("test.pdf", ios::binary);
			int count = 0;
			//chunk = new char[1024];
			while(!file.eof())  {
				int index = 1024;
				if(count != iterations)  {
					
					file.read(buffer,1024);
					messageSize[0] = '\0';
					messageSize[1] = '\0';
					// Send the packet size
					messageSize[0] = (uint8_t) ((index >> 8) & 0xff);
					messageSize[1] = (uint8_t) (index & 0xff);
					// Debug
					uint8_t msb = messageSize[0];
					uint8_t lsb = messageSize[1];
					uint16_t len = (msb << 8) | lsb;
					//cout << "Length:" << len << endl;
					int sent;
					sent = send(sockfd, messageSize, 2, 0);
					if (sent == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					//cout << "Bytes sent:" << sent << endl; 
					sent = send(sockfd, buffer, index, 0);
					// Send to previous ss
					if (sent == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					//cout << "Bytes sent:" << sent<< endl;
					
					count++;
				}
				else  {
					//cout << "Hello" << count << residue << endl;
					index = residue;
					//char *toSend = new char[index];
					file.read(buffer, index);
				
					// Send the packet size
					messageSize[0] = '\0';
					messageSize[1] = '\0';
					messageSize[0] = (uint8_t) ((index >> 8) & 0xff);
					messageSize[1] = (uint8_t) (index & 0xff);
					//cout << index << endl;
					if (send(sockfd, messageSize, 2, 0) == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					
					int sent = 0;
					sent = send(sockfd, buffer, index, 0);
					// Send to previous ss
					if (sent == -1)  { 
						printf("send\n");
						close(sockfd);
						exit(0);
					}
					//cout << "Bytes sent:" << sent << endl;
					break;
					count++;
				}
			
			}
			//file.close();
			// Delete temp file
			string del = "rm " + tempFile;
			system(del.c_str());
			cout << "File transmitted...Goodbye." << endl;
			//close(sockfd);
			return 0;
	}
	//delete[] messageSize;
	//delete[] buffer;
	return 0;
}
