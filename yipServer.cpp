#include "yip.h"

#include <thread>

int yip::serverFunc(void* pData)
{
	yip::CTCPServer srv;
	srv.Open();
	srv.Listen();
	std::thread recieveThread (CTCPServer::Recieve, &srv);
	//srv.Recieve();
	recieveThread.join();

	return 0;
}
