#pragma once

#ifdef YIP_LIB
#define YIP_API __declspec(dllexport)
#else
#define YIP_API __declspec(dllimport)
#endif

#include "types.h"
#include <atomic>

namespace yip
{
	//int serverFunc(void* pData);
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

	struct RxMsgBuffer
	{
		static const uti::uint32 m_kRxBufferLen = 1024 * 10;
		uti::u8 data [m_kRxBufferLen];
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


	class CTCPServer;

	struct ServerClient
	{
		CTCPServer* server;
		static const uti::u64 kMsgBufferCount = 10;
		RxMsgBuffer msg_queue[kMsgBufferCount];
		socket_t  socket;
		std::atomic<bool>      connected;
		uti::ptr  rx_thread;
		std::atomic<uti::u32>  idx_read;
		std::atomic<uti::u32>  idx_write;
		void (*rx_handler)(uti::u32 /*client_id*/);
	};

	class CTCPServer
	{
	public:
		static const uti::uint32 m_kDefaultPort = 8080;
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

		bool m_recieve;

		//!< The number of clients which are currently connected
		int m_iNumClients;

	public:
		//!< Array of clients
		ServerClient clients[m_kMaxConnections];

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
		* Recives incoming data from a client
		*/
		static void ClientRecieve(void* client);

		/**
		  * Responds to connected clients
		*/
		bool Repond( ServerClient* client, const RxMsgBuffer& msg );

		/**
		* Closes the given clients connection
		*/
		bool CloseClient(int client_id);

		/**
		  * Closes an already started server, returns true if the server was able to shutdown
		*/
		bool Close();
	};

	class YIP_API CTCPClient
	{
	private:
		//!< The default port if one is not supplied
		static const uti::uint32 m_kDefaultPort = 8080;
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
