#include "yip.h"

#include <thread>

int yip::serverFunc(void* pData)
{
	yip::CTCPServer srv;
	srv.Open();
	std::thread recieveThread(CTCPServer::Recieve, &srv);

	while(true) 
		srv.Listen();

	srv.Close();
	recieveThread.join();

	return 0;
}
