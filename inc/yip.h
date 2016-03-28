#pragma once

#ifdef YIP_LIB
#define YIP_API __declspec(dllexport)
#else
#define YIP_API __declspec(dllimport)
#endif

#include "types.h"

namespace yip
{
	int serverFunc(void* pData);
	int clientFunc(void* pData);

	typedef uti::ptr socket_t;

	enum MsgCompress : uti::uint8
	{
		compression_none
	};

	enum MsgEncrypt : uti::uint8
	{
		encrypt_none
	};

	struct MsgBuffer
	{
		uti::u8* data;
		uti::ptr length;
	};

	struct FileMsgHeader
	{
		uti::i64 id;
		uti::i64 version;
		uti::i64 size;
		MsgCompress compresssion;
		MsgEncrypt  encryption;
		char path[256];
	};

	struct FileDownload
	{
		uti::i64 id;
		uti::i64 version;
		uti::i64 total;
		uti::i64 recieved;
	};

	struct ServerClient
	{
		static const uti::u64 kMsgBufferCount = 10;
		MsgBuffer dataQueue[kMsgBufferCount];
		socket_t  socket;
		bool      connected;
	};

	class YIP_API CTCPServer
	{
	private:
		static const uti::uint32 m_kDefaultPort = 7000;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kRxBufferLen = 1024 * 10;
		//!< The maximum number of connection supported
		static const uti::uint32 m_kMaxConnections = 8;

	private:
		bool m_bOpen;

		//!< The buffer used to recieve data from the client sockets
		uti::int8 m_aRxBuffer[m_kRxBufferLen];

		//!< The socket for listening to incoming connections
		socket_t m_listen;

		ServerClient clients[m_kMaxConnections];

		//!< The number of clients which are currently connected
		int m_iNumClients;

		bool m_recieve;

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

	class YIP_API CTCPClient
	{
	private:
		//!< The default port if one is not supplied
		static const uti::uint32 m_kDefaultPort = 7000;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kRxBufferLen = 1024 * 10;
		//!< The size of the buffer for incoming data in bytes
		static const uti::uint32 m_kTxBufferLen = 1024 * 10;

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

		static void Recieve(void* pTcpClient);

		void Respond(const uti::u8* message, uti::ptr messageLength);

		/**
		  * Closes an open connection, returns true if the connection was successfully shutdown
		*/
		bool Close();
	};
}
