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

CTCPClient::CTCPClient() : m_bOpen(false), 
						   m_server(INVALID_SOCKET)
{
	// Initialize Winsock
	// TODO: move to dll main init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
	}
}

CTCPClient::~CTCPClient()
{
	// Shutdown Winsock
	// TODO: move to dll main shutdown
	WSACleanup();
}

bool CTCPClient::Open(const char* address, uti::uint32 port /*= m_kDefaultPort*/)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	char portStr[16];
	memset(&portStr[0], 0, 16);
	sprintf_s<16>(portStr, "%i", port);

	addrinfo* result;
	int iResult = getaddrinfo(address, portStr, &hints, &result);
	if (iResult != 0) 
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return false;
	}

	// Attempt to connect to an address until one succeeds
	for (addrinfo* pAdd = result; pAdd != NULL; pAdd = pAdd->ai_next)
	{
		// Create a SOCKET for connecting to server
		m_server = socket(pAdd->ai_family, pAdd->ai_socktype, pAdd->ai_protocol);
		if (m_server == INVALID_SOCKET) 
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return false;
		}

		iResult = connect(m_server, pAdd->ai_addr, (int)pAdd->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			closesocket(m_server);
			m_server = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (m_server == INVALID_SOCKET)
		return false;

	return true;
}

void CTCPClient::Recieve()
{
	int iResult = recv(m_server, &m_aRxBuffer[0], m_kRxBufferLen, 0);
	while (iResult > 0)
	{
		if (iResult > 0)
		{
			printf("Bytes received: %d\n", iResult);
			iResult = recv(m_server, &m_aRxBuffer[0], m_kRxBufferLen, 0);
		}
		else if (iResult == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
		}
	}
}

void CTCPClient::Respond()
{
	sprintf_s<m_kTxBufferLen>(m_aTxBuffer,"Stuff and things");
	int iResult = send(m_server, m_aTxBuffer, (int)strlen(m_aTxBuffer), 0);
	if (iResult == SOCKET_ERROR) 
	{
		printf("send failed with error: %d\n", WSAGetLastError());
	}
	else
	{
		printf("Bytes Sent: %ld\n", iResult);
	}
}

bool CTCPClient::Close()
{
	// shutdown the connection
	int iResult = shutdown(m_server, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(m_server);
		return false;
	}

	// read remaining data
	Recieve();

	// cleanup
	closesocket(m_server);

	return true;
}
