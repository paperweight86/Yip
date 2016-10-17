#include "yip.h"

#include "types.h"
#include "str.h"

#include <tchar.h>
#include <thread>
#include <assert.h>

using namespace uti;

yip::RxMsgBuffer g_responseMsg;

// TODO: [DanJ] XMACROS for less duplication
enum http_method : u8
{
	http_method_unknown = 255,

	http_method_get = 0,
	http_method_post,
	http_method_put,
	http_method_delete,

	http_method_count
};

const char* http_methods[http_method_count] =
{
	"GET",
	"POST",
	"PUT",
	"DELETE"
};

enum http_content_type
{
	http_content_unknown,

	http_content_text_html,
	http_content_text_plain,
};

const u64 http_max_path = 256;
const u64 http_max_host = 256;
const u64 http_user_agent_max = 256;
const u64 http_accept_max = 256;
const u64 http_accept_encoding_max = 256;
const u64 http_accept_language_max = 256;
const u64 http_max_conn_len = 256;

enum http_header_parts
{
	http_header_host,
	http_header_connection,
	http_header_cache_control,
	http_header_upgrade_insecure_requests,
	http_header_user_agent,
	http_header_accept,
	http_header_accept_encoding,
	http_header_accept_language,
};

//enum http_connection
//{
//	http_connection_keep_alive,
//};

struct http_request_cache_control
{
	u64 max_age;
	u64 max_stale;
	u64 min_fresh;
	bool no_cache;
	bool no_store;
	bool no_transform;
	bool only_if_cached;
};

struct http_header
{
	http_method method;
	char path[http_max_path];
	int major_version;
	int minor_version;
	http_content_type content_type;
	u64 content_length;
	u64 content_start;
	bool valid;

	char host[http_max_host];
	char connection[http_max_conn_len];
	http_request_cache_control cache_control;
	int upgrade_insecure_requests;
	char user_agent[http_user_agent_max];
	char accept[http_accept_max];
	char accept_encoding[http_accept_encoding_max];
	char accept_language[http_accept_language_max];
};

bool parse_http_header_part(const char* tag, u64 tag_len, const char* msg, u64& msg_pos_out, u64 msg_len, char* part_out, u64 part_out_max_len)
{
	if (strncmp(tag, msg + msg_pos_out, tag_len) == 0)
	{
		msg_pos_out += tag_len;
		msg_pos_out += str::find_not_char(msg + msg_pos_out, ' ', msg_len - msg_pos_out);
		if (msg_pos_out >= msg_len)
			return false;
		u64 end_offset = str::find_char(msg + msg_pos_out, '\r\n', msg_len - msg_pos_out);
		if (msg_pos_out + end_offset >= msg_len || end_offset == 0)
			return false;
		assert(part_out_max_len >= end_offset);
		memcpy(part_out, msg + msg_pos_out, end_offset);
		msg_pos_out += end_offset + 2;
	}
	else { return false; }

	return true;
}

void parse_http_header(const char* msg, u64 msg_len, http_header* header)
{
	u64 msg_pos = 0;
	header->valid = false;
	
	// Method
	header->method = http_method_unknown;
	for (u8 i = 0; i < http_method_count; ++i)
	{
		u8 len = strlen(http_methods[i]);
		assert(msg_len >= len);
		if (strncmp(msg, http_methods[i], len) == 0)
		{
			header->method = (http_method)i;
			msg_pos += len;
			break;
		}
	}

	// Path
	u64 path_start_pos = str::find_char( msg + msg_pos, ' ', msg_len - msg_pos );
	path_start_pos += 1;
	if (path_start_pos >= msg_len)
		return;

	u64 path_size = str::find_char(msg + msg_pos + path_start_pos, ' ', msg_len - msg_pos);
	path_size += 1;
	if (path_size >= msg_len)
		return;

	assert(path_size < http_max_path);
	header->path[path_size] = 0;
	memcpy(header->path, msg + msg_pos + path_start_pos, path_size);

	msg_pos += path_start_pos + path_size;

	if (path_size >= msg_pos)
		return;

	// Protocal
	u64 protocal_end = str::find_char(msg + msg_pos, '\r\n', msg_len - msg_pos);
	if (protocal_end >= msg_len)
		return;

	const char* protocal_http_start = "HTTP/";
	const u64 protocal_http_start_len = strlen(protocal_http_start);
	if (strncmp(protocal_http_start, msg + msg_pos, protocal_http_start_len) != 0)
		return;
	msg_pos += protocal_http_start_len;

	const u64 ver_max_len = 6;
	char version[ver_max_len] = {};
	assert(ver_max_len >= protocal_end - protocal_http_start_len);
	memcpy(version, msg + msg_pos, protocal_end - protocal_http_start_len);

	u64 dot_pos = str::find_char(version, '.', ver_max_len);
	if (dot_pos >= ver_max_len)
		return;
	version[dot_pos] = 0;
	header->minor_version = atoi(version);
	version[dot_pos+2] = 0;
	header->major_version = atoi(version+dot_pos+1);

	msg_pos += dot_pos + 2;

	u64 line_end_off = str::find_char(msg + msg_pos, '\r\n', msg_len - msg_pos);
	if (line_end_off >= msg_len)
		return;
	msg_pos += line_end_off + 2;

	header->valid = true;

	// Headers

	const char* host_tag = "Host:";
	const u64 host_tag_len = strlen(host_tag);
	if (!parse_http_header_part(host_tag, host_tag_len, msg, msg_pos, msg_len, header->host, http_max_host))
		return;

	if (msg_pos >= msg_len)
		return;

	//http_request_cache_control cache_control;
	//int upgrade_insecure_requests;
	//char user_agent[http_user_agent_max];
	//char accept[http_accept_max];
	//char accept_encoding[http_accept_encoding_max];
	//char accept_language[http_accept_language_max];

	const char* conn_tag = "Connection:";
	const u64 conn_tag_len = strlen(conn_tag);
	if (!parse_http_header_part(conn_tag, conn_tag_len, msg, msg_pos, msg_len, header->connection, http_max_conn_len))
		return;
	if (msg_pos >= msg_len)
		return;
	
	// Cache Control
	{
		const char* cache_ctrl_tag = "Cache-Control:";
		const u64 cache_ctrl_tag_len = strlen(cache_ctrl_tag);
		const u64 cache_ctrl_buffer_len = 1024;
		char cache_ctrl_buffer[cache_ctrl_buffer_len] = {};
		if (!parse_http_header_part(cache_ctrl_tag, cache_ctrl_tag_len, msg, msg_pos, msg_len, cache_ctrl_buffer, cache_ctrl_buffer_len))
			return;
		if (msg_pos >= msg_len)
			return;

		// TODO: [DanJ] Indicates how many variables (maybe?)
		//str::find_num_char(cache_ctrl_buffer, ';', strlen(cache_ctrl_buffer));
		bool more_vars = false;
		do
		{
			u64 no_ws_offset = str::find_not_char(cache_ctrl_buffer, ' ', cache_ctrl_buffer_len);
			if (no_ws_offset > cache_ctrl_buffer_len)
				break;



		} while (more_vars);
	}

	//header->host = ;

	//Cache-Control: max-age=0
	//Upgrade-Insecure-Requests: 1
	//User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.116 Safari/537.36
	//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*; q = 0.8
	//Accept - Encoding: gzip, deflate, sdch
	//Accept - Language : en - GB, en - US; q = 0.8, en; q = 0.6


}

void ReadClientData(void* pInput)
{
	yip::CTCPServer* srv = (yip::CTCPServer*)pInput;
	while (1)
	{
		for (int i = 0; i < yip::CTCPServer::m_kMaxConnections; ++i)
		{
			if (srv->clients[i].connected && srv->clients[i].idx_read < uint32_max && srv->clients[i].idx_read != srv->clients[i].idx_write)
			{
				yip::RxMsgBuffer* msg = srv->clients[i].msg_queue + srv->clients[i].idx_read++;
				printf("Message of %d bytes recieved from client %d\n", msg->length, i);
				char buff[255] = {};
				sprintf(buff, "Message content %s%i%s", "%", msg->length, "s");
				printf(buff, msg->data);
				http_header header = {};
				parse_http_header((const char*)msg->data, msg->length, &header);
				sprintf((char *)g_responseMsg.data, "%s", "\
HTTP/1.1 200 OK\r\n\
Date: Sun, 18 Sep 2016 17:10:00 GMT\r\n\
Content-Type: text/html\r\n\
Content-Length: 40\r\n\
\r\n\
<html>\r\n\
<body>\r\n\
Test\r\n\
</body>\r\n\
</html>\r\n\
");
				g_responseMsg.length = strlen((const char*)g_responseMsg.data);

				if (srv->Repond(srv->clients + i, g_responseMsg))
					printf("Sent message to client %d\n%s\n", i, g_responseMsg.data);

				//srv->CloseClient(i);

				//for (int j = 0; j < yip::CTCPServer::m_kMaxConnections; ++j)
				//{
				//	if(srv->clients[j].connected)
				//		if (srv->Repond(srv->clients + j, g_responseMsg))
				//			printf("Sent message to client %d\n%s\n", j, g_responseMsg);
				//}
			}
		}
	}
}

int serverFunc(void* pData)
{
	yip::CTCPServer srv;
	srv.Open();
	std::thread recieveThread(yip::CTCPServer::Recieve, &srv);

	std::thread dataReadThread(ReadClientData, &srv);

	while (true)
		srv.Listen();

	srv.Close();
	recieveThread.join();
	dataReadThread.join();

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int32 iReturn = serverFunc(nullptr);

	system("PAUSE");

	return iReturn;
}
