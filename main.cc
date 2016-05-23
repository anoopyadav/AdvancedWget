// main.cc
// 
// Copyright 2013 anzy <anzy@ubuntu>
// 
// 
// 


#include <iostream>
#include "ss.h"
int main(int argc, char** argv)  {
	SteppingStone server;
	server.parseArguments(argc, argv);
	server.startServer();
	return 0;
}


