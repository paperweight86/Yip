#include "yip.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <thread>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "7000"

using namespace yip;

int CYip::serverFunc(void* pData)
{
	CTCPServer srv;
	srv.Open();
	srv.Listen();
	std::thread recieveThread (CTCPServer::Recieve, &srv);
	//srv.Recieve();
	recieveThread.join();

	return 0;
}
