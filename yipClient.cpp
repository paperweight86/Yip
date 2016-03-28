#include "yip.h"

#include <iostream>
#include <thread>

int yip::clientFunc(void* pData)
{
	yip::CTCPClient cli;
	while(!cli.Open("localhost")) {}
	
	std::thread recieveThread (CTCPClient::Recieve, &cli);
	
	std::string message = "";
	while (message != "q")
	{
		//std::cin >> message;
		std::getline(std::cin, message);
		cli.Respond((const uti::u8*)message.c_str(), message.length()+1);
	}
	cli.Close();

	return 0;
}
