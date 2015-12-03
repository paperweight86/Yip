#pragma once

#ifdef YIP_LIB
#define YIP_API __declspec(dllexport)
#else
#define YIP_API __declspec(dllimport)
#endif

#include "types.h"

namespace yip
{
	class YIP_API CYip
	{
	public:
		static int serverFunc(void* pData);
		static int clientFunc(void* pData);
	};

	//!< Type of a socket (ptr)
	typedef uti::ptr socket_t;

	class YIP_API CTCPServer
	{
	private:
		//!< The default port if one is not supplied
		static const uti::uint32 m_kDefaultPort = 7000;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kRxBufferLen = 512;
		//!< The maximum number of connection supported
		static const uti::uint32 m_kMaxConnections = 8;

	private:
		bool m_bOpen;

		//!< The buffer used to recieve data from the client sockets
		uti::int8 m_aRxBuffer[m_kRxBufferLen];

		//!< The socket for listening to incoming connections
		socket_t m_listen;

		//!< The sockets representing connected clients
		socket_t m_aClients[m_kMaxConnections];

		//!< Which clients are connected
		bool  m_aConnected[m_kMaxConnections];

		//!< The number of clients which are currently connected
		int m_iNumClients;

	public:
		CTCPServer();
		~CTCPServer();

		/**
		  * Opens a server on the given port, returns true if the server was setup sucessfully
		*/
		bool Open( uti::uint32 listenPort = m_kDefaultPort );

		/**
		  * Blocks and listens (i.e. waits) for a new connection, returns true for a successful connection
		*/
		bool Listen();

		/**
		  * Recives incoming data
		*/
		static void Recieve(void* pTcpServer);

		/**
		  * Responds to connected clients
		*/
		void Repond();

		/**
		  * Closes an already started server, returns true if the server was able to shutdown
		*/
		bool Close();
	};

	class CTCPClient
	{
	private:
		//!< The default port if one is not supplied
		static const uti::uint32 m_kDefaultPort = 7000;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kRxBufferLen = 512;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kTxBufferLen = 512;

	private:
		//!< Whether or not a connection is currently open
		bool m_bOpen;

		//!< Socket representing the connection with the server
		socket_t m_server;

		//!< The buffer used to recieve data from the server socket
		uti::int8 m_aRxBuffer[m_kRxBufferLen];

		//!< The buffer used to send data to the server socket
		uti::int8 m_aTxBuffer[m_kTxBufferLen];

	public:
		CTCPClient();
		~CTCPClient();

		/**
		  * Opens a connection to the provided address on the given port, returns true if successful
		*/
		bool Open(const char* address, uti::uint32 port = m_kDefaultPort);

		void Recieve( );

		void Respond();

		/**
		  * Closes an open connection, returns true if the connection was successfully shutdown
		*/
		bool Close();
	};
}
