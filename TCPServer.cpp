#include "yip.h"

using namespace uti;
using namespace yip;

#undef UNICODE
//!< this define allows the use of winsock2.h
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

CTCPServer::CTCPServer() : m_listen(INVALID_SOCKET), 
						   m_iNumClients(0), 
						   m_bOpen(false)
{
	memset(&m_aRxBuffer[0],  0, m_kRxBufferLen);
	memset(&clients, 0, m_kMaxConnections * sizeof(ServerClient));

	// Initialize Winsock
	// TODO: move to dll main init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
	}
	m_recieve = false;
}

CTCPServer::~CTCPServer()
{
	if (m_bOpen)
		Close();

	// Shutdown Winsock
	// TODO: move to dll main shutdown
	WSACleanup();
}

bool CTCPServer::Open(uti::uint32 listenPort /*= m_kDefaultPort*/)
{
	if (m_bOpen)
		return false;

	m_bOpen = true;

	addrinfo  hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	char portStr[16];
	memset(&portStr[0], 0, 16);
	sprintf_s<16>(portStr, "%i", listenPort);

	addrinfo* addr = NULL;
	int iResult = getaddrinfo(NULL, portStr, &hints, &addr);
	if (iResult != 0) 
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return false;
	}

	// Create listen socket
	m_listen = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (m_listen == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(addr);
		WSACleanup();
		return false;
	}

	// Setup listening socket
	iResult = bind(m_listen, addr->ai_addr, (int)addr->ai_addrlen);
	if (iResult == SOCKET_ERROR) 
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addr);
		closesocket(m_listen);
		WSACleanup();
		return false;
	}

	// Clean up
	freeaddrinfo(addr);

	return true;
}

bool CTCPServer::Listen() 
{
	int iResult = listen(m_listen, SOMAXCONN);
	if (iResult == SOCKET_ERROR) 
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(m_listen);
		return false;
	}

	// Accept a client socket
	sockaddr clientAddr {};

	socket_t client = accept(m_listen, &clientAddr, NULL);
	if (client == INVALID_SOCKET) 
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		return false;
	}

	struct sockaddr_in *ipv4 = (struct sockaddr_in *)&clientAddr;
	char ipAddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddress, INET_ADDRSTRLEN);
	int i = -1;
	for (i = 0; i < m_kMaxConnections; ++i)
	{
		if (!clients[i].connected)
		{
			clients[i].socket = client;
			clients[i].connected = true;
			++m_iNumClients;
			
			printf("Client Connected  %s %d\n", ipAddress, ipv4->sin_port);
			break;
		}
	}

	if (i == -1)
		return false;

	return true;
}

void CTCPServer::Recieve( void* pTcpServer )
{
	CTCPServer* srv = reinterpret_cast<CTCPServer*>(pTcpServer);
	srv->m_recieve = true;
	while (srv->m_recieve)
	{
		for (int i = 0; i < srv->m_kMaxConnections; ++i)
		{
			if (!srv->clients[i].connected)
				continue;

			int iResult = recv(srv->clients[i].socket, &srv->m_aRxBuffer[0], srv->m_kRxBufferLen, 0);
			if (iResult > 0)
			{
				printf("Bytes received: %d\n", iResult);

				printf(">> %s\n", (const char*)&srv->m_aRxBuffer[0]);

				// Echo the buffer back to the sender
				//int iSendResult = send(srv->clients[i].socket, &srv->m_aRxBuffer[0], iResult, 0);
				//if (iSendResult == SOCKET_ERROR)
				//{
				//	printf("send failed with error: %d\n", WSAGetLastError());
				//	closesocket(srv->clients[i].socket);
				//	srv->clients[i].connected = false;
				//	--srv->m_iNumClients;
				//}
				//printf("Bytes sent: %d\n", iSendResult);

				FILE* file = nullptr;
				fopen_s(&file, (const char*)&srv->m_aRxBuffer[0], "r");
				uti::u8* payload = nullptr;
				i64 flen = 0;
				if (file != NULL)
				{
					fseek(file, 0, SEEK_END);
					flen = _ftelli64(file);
					fseek(file, 0, SEEK_SET);
					payload = new uti::u8[flen];
					fread((void*)payload, flen, 1, file);
					fclose(file);
				}
				
				if (payload != nullptr)
				{
					const char* header = "FILESTART";
					const char* footer = "FILEEND";

					send(srv->clients[i].socket, header, strlen(header), 0);
					send(srv->clients[i].socket, (const char*)&flen, sizeof(i64), 0);

					//int iSendResult = send(srv->clients[i].socket, &srv->m_aRxBuffer[0], iResult, 0);
					//i64 maxSendSize = 1024 * 1024; // 1 MB
					i64 numSends = (i64)((float)flen / (float)srv->m_kRxBufferLen + 0.5f);
					i64 bytesSent = 0;
					//for (int j = 0; j < numSends; ++j)
					while(bytesSent < flen)
					{
						i64 sendLen = bytesSent + srv->m_kRxBufferLen <= flen? srv->m_kRxBufferLen
																			 : flen - bytesSent;
						int sendResult = send(srv->clients[i].socket, (const char*)payload, sendLen, 0);
						if (sendResult == SOCKET_ERROR)
						{
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(srv->clients[i].socket);
							srv->clients[i].connected = false;
							--srv->m_iNumClients;
							break;
						}
						bytesSent += sendLen;
						payload += sendLen;
						printf("file progress: %d\n", bytesSent);
					}

					payload -= bytesSent;
					delete [] payload;
					payload = nullptr;
				}

				memset(srv->m_aRxBuffer, 0, srv->m_kRxBufferLen);
			}
			else if (iResult == 0)
			{
				printf("Connection closing...\n");
				closesocket(srv->clients[i].socket);
				srv->clients[i].connected = false;
				--srv->m_iNumClients;
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(srv->clients[i].socket);
				srv->clients[i].connected = false;
				--srv->m_iNumClients;
			}
		}
	};
}

void CTCPServer::Repond()
{

}

bool CTCPServer::Close()
{
	if (!m_bOpen)
		return false;

	m_recieve = false;

	closesocket(m_listen);

	if (m_iNumClients > 0)
	{
		for (int i = 0; i < m_kMaxConnections; ++i)
		{
			if (!clients[i].connected)
				continue;

			int iResult = shutdown(clients[i].socket, SD_SEND);
			if (iResult == SOCKET_ERROR)
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(clients[i].socket);
				clients[i].connected = false;
				--m_iNumClients;
			}
		}
	}

	return true;
}
