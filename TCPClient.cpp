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

#include <assert.h>

#include <io.h>

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
			return false;
		}

		// Set the outgoing port
		sockaddr_in client_addr {};
		memset(&client_addr, 0, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(7500 + rand()%100);

		//bind(s, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
		//iResult = bind(m_server, (const sockaddr*)&client_addr, sizeof(client_addr));
		//if (iResult == SOCKET_ERROR)
		//{
		//	printf("bind failed with error: %d\n", WSAGetLastError());
		//	closesocket(m_server);
		//	return false;
		//}
		
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)pAdd->ai_addr;
		char ipAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddress, INET_ADDRSTRLEN);
		printf("Connecting to %s\n", ipAddress);
		iResult = connect(m_server, pAdd->ai_addr, (int)pAdd->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			printf("Connectiong failed!\n");
			closesocket(m_server);
			m_server = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (m_server == INVALID_SOCKET)
		return false;

	memset(&m_aRxBuffer[0], 0, m_kRxBufferLen);

	return true;
}

void CTCPClient::Recieve(void* pTcpClient)
{
	auto pClient = (CTCPClient*)pTcpClient;

	bool recievingFile = false;
	bool readSize = false;
	uti::u8* payload = nullptr;
	FILE* file = nullptr;
	i64 flen = 0;
	i64 read = 0;
	float percentage = 0.0f;
	u32 lastT = 0;
	u32 startT = 0;

	int iResult = recv(pClient->m_server, &pClient->m_aRxBuffer[0], m_kRxBufferLen, 0);
	while (iResult > 0)
	{
		if (iResult > 0)
		{
			//printf("Bytes received: %d\n", iResult);

			if (recievingFile)
			{				
				i64 consumed = 0;
				if (readSize)
				{
					startT = GetTickCount();
					if (payload != nullptr)
						delete[] payload;

					flen = *((i64*)&pClient->m_aRxBuffer[0]);
					payload = new uti::u8[flen];
					readSize = false;
					consumed = sizeof(i64);
					printf("File is %d bytes...\n", flen);
					printf("Allocating disk space...\n");
					auto err = fopen_s(&file, "C:\\temp\\recieved.zip", "wb+");
					HANDLE osfHandle = (HANDLE)_get_osfhandle(_fileno(file));
					FlushFileBuffers(osfHandle);
					DWORD high = flen >> 32;
					DWORD low = (DWORD)flen;
					HANDLE h = ::CreateFileMapping(osfHandle, 0, PAGE_READWRITE, high, low, 0);
					DWORD dwError;
					if (h == (HANDLE)NULL)
						dwError = GetLastError();
					LARGE_INTEGER tempLrgInt;
					tempLrgInt.QuadPart = flen;
					SetFilePointerEx(h, tempLrgInt, 0, FILE_BEGIN);
					SetEndOfFile(h);
					tempLrgInt.QuadPart = 0;
					SetFilePointerEx(h, tempLrgInt, 0, FILE_BEGIN);
					fclose(file);
					CloseHandle(h);
					printf("Awaiting data...\n");
					err = fopen_s(&file, "C:\\temp\\recieved.zip", "rb+");
					//fseek(file, 0, FILE_BEGIN);
				}
				
				{
					auto written = fwrite(&pClient->m_aRxBuffer[0] + consumed, iResult - consumed, 1, file);
					//fclose(file);
					read += iResult - consumed;

					percentage = (float)read / (float)flen * 100.0f;
					//memcpy(payload, &pClient->m_aRxBuffer[0] + consumed, iResult - consumed);
					//payload += iResult - consumed;
					//read += iResult - consumed;
					if (read >= flen)
				    {
						fclose(file);
						recievingFile = false;
						readSize = false;
						flen = 0;
						read = 0;
						iResult = 0;
						consumed = 0;
					}
					//{
					//	recievingFile = false;
					//	payload -= flen;
					//	auto err = fopen_s(&file, "C:\\temp\\recieved.jpg", "wb");
					//	fwrite(payload, flen, 1, file);
					//	fclose(file);
					//	flen = 0;
					//	read = 0;
					//	delete [] payload;
					//	payload = nullptr;
					//}
				}
				//printf("File Progress: %d\n", read);
				if (GetTickCount() - lastT > 500 || !recievingFile)
				{
					lastT = GetTickCount();
					printf("%d \%\r", (int)percentage);
					if (!recievingFile)
						printf("\n");
				}
			}
			else
			{
				const char* header = "FILESTART";
				if (iResult == strlen(header) && strcmp((const char*)&pClient->m_aRxBuffer[0], header) == 0)
				{
					printf("Recieving file...\n");
					recievingFile = true;
					readSize = true;
				}
			}

			//printf(">> %s\n", (const char*)&pClient->m_aRxBuffer[0]);
			//
			//memset(&pClient->m_aRxBuffer[0], 0, pClient->m_kRxBufferLen);

			iResult = recv(pClient->m_server, &pClient->m_aRxBuffer[0], pClient->m_kRxBufferLen, 0);
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

void CTCPClient::Respond( const u8* message, ptr messageLength )
{
	//sprintf_s<m_kTxBufferLen>(m_aTxBuffer, message);
	memcpy_s(m_aTxBuffer, m_kTxBufferLen, message, messageLength);
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
	Recieve(this);

	// cleanup
	closesocket(m_server);

	return true;
}
