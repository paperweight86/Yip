#include "yip.h"

#include <thread>

//void ReadClientData(void* pInput)
//{
//	yip::CTCPServer* srv = (yip::CTCPServer*)pInput;
//	while (1)
//	{
//		for (int i = 0; i < yip::CTCPServer::m_kMaxConnections; ++i)
//		{
//			if (srv->clients[i].connected && srv->clients[i].idx_read < uint32_max && srv->clients[i].idx_read != srv->clients[i].idx_write)
//			{
//				uti::u8* pData = srv->clients[i].msg_queue + (srv->clients[i].idx_read++ * i * yip::CTCPServer::m_kRxBufferLen);
//			}
//		}
//	}
//}
//
//int yip::serverFunc(void* pData)
//{
//	yip::CTCPServer srv;
//	srv.Open();
//	std::thread recieveThread(yip::CTCPServer::Recieve, &srv);
//
//	std::thread dataReadThread(ReadClientData, &srv);
//
//	while(true)
//		srv.Listen();
//
//	srv.Close();
//	recieveThread.join();
//	dataReadThread.join();
//
//	return 0;
//}
