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
	memset(&m_aClients[0],   0, m_kMaxConnections);
	memset(&m_aConnected[0], 0, m_kMaxConnections);

	// Initialize Winsock
	// TODO: move to dll main init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
	}
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
	socket_t client = accept(m_listen, NULL, NULL);
	if (client == INVALID_SOCKET) 
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		return false;
	}

	int i = -1;
	for (i = 0; i < m_kMaxConnections; ++i)
	{
		if (!m_aConnected[i])
		{
			m_aClients[i] = client;
			m_aConnected[i] = true;
			++m_iNumClients;
			printf("client connected");
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

	bool bClients = srv->m_iNumClients > 0;
	while (bClients)
	{
		for (int i = 0; i < srv->m_kMaxConnections; ++i)
		{
			if (!srv->m_aConnected[i])
				continue;

			int iResult = recv(srv->m_aClients[i], &srv->m_aRxBuffer[0], srv->m_kRxBufferLen, 0);
			if (iResult > 0)
			{
				printf("Bytes received: %d\n", iResult);

				// Echo the buffer back to the sender
				int iSendResult = send(srv->m_aClients[i], &srv->m_aRxBuffer[0], iResult, 0);
				if (iSendResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(srv->m_aClients[i]);
					srv->m_aConnected[i] = false;
					--srv->m_iNumClients;
				}
				printf("Bytes sent: %d\n", iSendResult);
			}
			else if (iResult == 0)
			{
				printf("Connection closing...\n");
				closesocket(srv->m_aClients[i]);
				srv->m_aConnected[i] = false;
				--srv->m_iNumClients;
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(srv->m_aClients[i]);
				srv->m_aConnected[i] = false;
				--srv->m_iNumClients;
			}
		}
		bClients = srv->m_iNumClients > 0;
	};
}

void CTCPServer::Repond()
{

}

bool CTCPServer::Close()
{
	if (!m_bOpen)
		return false;

	closesocket(m_listen);

	if (m_iNumClients > 0)
	{
		for (int i = 0; i < m_kMaxConnections; ++i)
		{
			if (!m_aConnected[i])
				continue;

			int iResult = shutdown(m_aClients[i], SD_SEND);
			if (iResult == SOCKET_ERROR)
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(m_aClients[i]);
				m_aConnected[i] = false;
				--m_iNumClients;
			}
		}
	}

	return true;
}
