#include "yip.h"

int yip::clientFunc(void* pData)
{
	yip::CTCPClient cli;
	while(!cli.Open("localhost")) {}
	cli.Respond();
	cli.Recieve();
	cli.Close();

	return 0;
}
