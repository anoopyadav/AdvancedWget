// server.h.cxx
// 
// Copyright 2013 anzy <anzy@ubuntu>
// 
// 
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
class SteppingStone  {
	private:
		std::string port;
		std::string url;
		std::vector<std::string> chainlist;
	public:
		SteppingStone();
		~SteppingStone();
		int parseArguments(int argc, char** argv);
		int getIpAddress();
		int startServer();
		int handleConnection(int sockfd);
};

