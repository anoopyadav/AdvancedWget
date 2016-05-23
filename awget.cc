// awget.cc.cxx
// 
// Copyright 2013 anzy <anzy@ubuntu>
// 
// 
// 


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>

using namespace std;
class AwgetClient  {
	private:
		ifstream chainfile;
		string url;
		vector<string> chainlist;
		string ssList;
		char buffer[1024];
		char hostname[16], port[6];
		int numberOfStones;
		
	public:
		AwgetClient();
		~AwgetClient();
		int parseArguments(int argc, char** argv);
		int readSteppingStones();
		int startClient();
		bool fileExists(const string& filename);
};

bool AwgetClient::fileExists(const string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}
AwgetClient::AwgetClient()  {
	// Seed the random number generator
	srand(time(NULL));
}

AwgetClient::~AwgetClient()  {
	if(chainfile.is_open())  {
		chainfile.close();
	}
}

int AwgetClient::parseArguments(int argc, char** argv)  {
	
	// Parse the command-line arguments
	if(argc == 1)  {
		cout << "Usage: awget <URL> [-c chainfile]" << endl;
		return 1;
	}
	if(argc == 2)  {
		url = argv[1];
		chainfile.open("chaingang.txt");
	}
	if(argc > 2)  {
		url = argv[1];
		if(strcmp(argv[2], "-c") != 0)  {
			cout << "Invalid switch!" << endl;
			return 1;
		}
		if(argc == 3)  {
			cout << "Invalid filename!" << endl;
			return 1;
		}
		chainfile.open(argv[3]);
	}
	
	// Check if file is ready
	if(!chainfile.is_open())  {
		cout << "Can't open chainfile!" << endl;
		return 1;
	}
	
	// Validate URL
	string command = "wget --spider -q " + url;
	int status = system(command.c_str());
	//cout << "Return:" << status << endl;
	if(status != 0)  {
		cout << "Destination does not exist!" << endl;
		return 1;
	}
	
	
	return 0;
}

int AwgetClient::readSteppingStones()  {
	string line;
	ostringstream out;
	getline(chainfile, line);
	numberOfStones = atoi(line.c_str());
	cout << "Chainlist is" << endl;
	
	while(getline(chainfile, line))  {
		//cout << line << endl;
		chainlist.push_back(line);
		cout << "<" << line << ">" << endl;
		out << line << " ";
	} 
	ssList = out.str();

	// Pick a random ss to send the request to
	// Pick a stepping stone at random
	if(chainlist.size() > 1)  {
		int index = rand() % chainlist.size();
		//cout << index << " " << chainlist[index] << endl;
		
		// Obtain connection information
		string target = chainlist[index];
		char *pch = strtok ((char*)target.c_str(),",");
		strcpy(hostname, pch);
		pch = strtok (NULL, " ");
		strcpy(port, pch);
		//cout << hostname << " " << port << endl;
		
		// Remove the chosen ss
		vector<string>::iterator it = std::find(chainlist.begin(), chainlist.end(), chainlist[index]);
		//if (it != chainlist.end()) chainlist.erase(it);
		chainlist.erase(it);
		//cout << chainlist.size() << endl;
		
		// Obtain new ssList
		ssList = "";
		for(int i = 0; i < chainlist.size(); i++)  {
			ssList += chainlist[i];
			if(i < chainlist.size() - 1)  {
				ssList += " ";
			}
		}
	}
	else  {
		// Obtain connection information
		string target = chainlist[0];
		char *pch = strtok ((char*)target.c_str(),",");
		strcpy(hostname, pch);
		pch = strtok (NULL, " ");
		strcpy(port, pch);
		
		// Remove the entry
		chainlist.pop_back();
		ssList = "";
	}
	cout << "Next ss is <" << hostname << "," << port << ">" << endl;
	
	
	// Prepare the data packet
	uint16_t totalLength = ssList.length() + url.length();
	//buffer = new char[totalLength + 5];
	uint16_t length = url.length();
	buffer[0] = (uint8_t) ((length >> 8) & 0xff);
	buffer[1] = (uint8_t) (length & 0xff);
	strncpy(buffer + 2, url.c_str(), (size_t)(length));
	length = ssList.length();
	buffer[url.length() + 2] = (uint8_t) ((length >> 8) & 0xff);
	buffer[url.length() + 3] = (uint8_t) (length & 0xff);
	strncpy(buffer + url.length() + 4, ssList.c_str(), (size_t)(length));
	buffer[ssList.length() + url.length() + 4 + 1] = '\0';
	//cout << buffer + 2 << " " << buffer + url.length() + 4 << endl;
	
	/*uint8_t msb, lsb;
	msb = buffer[0];
	lsb = buffer[1];
	uint16_t len =  (msb << 8) | lsb;
	cout << len << ssList.length() << endl;*/
	return 0;
}

int AwgetClient::startClient()  {
	int sockfd, numbytes, newfd;
	//char buffer[2];
	struct addrinfo hints, *serverInfo, *p;
	int rv;
	char ipAddress[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //always the same. At least in this class
	hints.ai_socktype = SOCK_STREAM; //set tcp
	
	
	//cout << hostname << " " << port << endl;
	
	//get information about the host and port specified by the user
	if ((rv = getaddrinfo(hostname, port, &hints, &serverInfo)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = serverInfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	freeaddrinfo(serverInfo); // all done with this structure
	//cout << buffer +2 << endl;
	// Send Request
	if (send(sockfd, buffer, url.length() + ssList.length() + 4, 0) == -1)  { 
		printf("send");
		close(sockfd);
		exit(0);
	}
	//delete[] buffer;
	//cout << buffer1 << endl;
	cout << "Waiting for file..." << endl;
	
	// Recieve filename
	//char *buffer;
	//buffer = new char[2];
	if ((numbytes = recv(sockfd, buffer, 2, 0)) == -1) {
		printf("\nError recieving message!\n");
		exit(1);
	}
	uint8_t msb = buffer[0];
	uint8_t lsb = buffer[1];
	uint16_t len =  (msb << 8) | lsb;
	//cout << len  << endl;
	//delete[] buffer;
	//buffer = new char[len + 1];
	//cout << buffer1 + 2 << endl;
	int bytes;
	if ((bytes = recv(sockfd, buffer, len, 0)) == -1) {
		printf("\nError recieving message!\n");
		exit(1);
	}
	buffer[len] = '\0';
	string filename = buffer;
	cout << "Filename:" << filename << endl;
	//delete[] buffer;
	
	// Check if file already exists
	bool exists = fileExists(filename);
	if(exists)  {
		cout << "Filename collision!" << endl;
		sleep(2);
		srand(time(NULL));
		int timestamp = time(NULL);
		filename = to_string(timestamp) + "_" + filename;
	}
	//filename += ".new";
	// Open a file to write
	ofstream out(filename, ios::binary);
	//buffer = new char[2];
	char messageBuffer[1024];
	char messageLength[2];
	// Loop to read the file
	//while(recv(sockfd, buffer, 1024, 0) != -1)  {
	int recieved;
	int counter = 5;
	while((recieved = read(sockfd, messageLength, 2)) > 0)  {
		if(counter == 5)  {
			counter = 0;
			sleep(2);
		}
		//cout << "Received" << endl;
		if (recieved == 0)  {
			break;
		}
		// Read the length
		msb = messageLength[0];
		lsb = messageLength[1];
		len =  (msb << 8) | lsb;
		
		recieved = read(sockfd, messageBuffer, len);
		out.write(messageBuffer, len);

		if(len < 1024)  {
			close(sockfd);
			break;
		}
		counter++;
		
	}
	out.close();
	//close(sockfd);
	cout << "File received...Goodbye." << endl;
	return 0;

}

int main(int argc, char** argv)  {
	AwgetClient awget;
	if(awget.parseArguments(argc, argv) != 0)  {
		cout << "Exiting..." << endl;
		return 0;
	} 
	if(awget.readSteppingStones() != 0)  {
		cout << "File format error!" << endl;
		return 1;
	} 
	awget.startClient();
	return 0;
}

