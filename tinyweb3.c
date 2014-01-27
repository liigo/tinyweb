#include "tinyweb3.h"
#include "membuf.h"
#include <uv.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <memory.h>

//Tinyweb v3, a tiny web server based on libuv, by liigo, 2013-6-30.

uv_tcp_t    _server;
uv_tcp_t    _client;
uv_loop_t*  _loop = NULL;
const char* _doc_root_path = NULL;

static void tinyweb_on_connection(uv_stream_t* server, int status);

//start web serer, linstening ip:port
//ip can be NULL or "", which means "0.0.0.0"
//doc_root_path can be NULL, or requires not end with /
void tinyweb_start(uv_loop_t* loop, const char* ip, int port, const char* doc_root_path) {
	struct sockaddr_in addr;
	uv_ip4_addr((ip && ip[0]) ? ip : "0.0.0.0", port, &addr);

	_loop = loop;
	if(doc_root_path && doc_root_path[0])
		_doc_root_path = doc_root_path;

	uv_tcp_init(_loop, &_server);
	uv_tcp_bind(&_server, (const struct sockaddr*) &addr);
	uv_listen((uv_stream_t*)&_server, 8, tinyweb_on_connection);
}

static void after_uv_close_client(uv_handle_t* client) {
	membuf_uninit((membuf_t*)client->data); //see: tinyweb_on_connection()
	free(client->data); //membuf_t*: request buffer
	free(client);
}

static void after_uv_write(uv_write_t* w, int status) {
	if(w->data)
		free(w->data); //copyed data
	uv_close((uv_handle_t*)w->handle, after_uv_close_client);
	free(w);
}

//close the connect of a client
static void tinyweb_close_client(uv_stream_t* client) {
	uv_close((uv_handle_t*)client, after_uv_close_client);
}

//write data to client
//data: the data to be sent
//len:  the size of data, can be -1 if data is string
//need_copy_data: copy data or not
//need_free_data: free data or not, ignore this arg if need_copy_data is non-zero
//write_uv_data() will close client connection after writien success, because tinyweb use short connection.
static void write_uv_data(uv_stream_t* client, const void* data, unsigned int len, int need_copy_data, int need_free_data) {
	uv_buf_t buf;
	uv_write_t* w;
	void* newdata  = (void*)data;

	if(data == NULL || len == 0) return;
	if(len ==(unsigned int)-1)
		len = strlen((char*)data);

	if(need_copy_data) {
		newdata = malloc(len);
		memcpy(newdata, data, len);
	}

	buf = uv_buf_init((char*)newdata, len);
	w = (uv_write_t*)malloc(sizeof(uv_write_t));
	w->data = (need_copy_data || need_free_data) ? newdata : NULL;
	uv_write(w, client, &buf, 1, after_uv_write); //free w and w->data in after_uv_write()
}

//status: "200 OK"
//content_type: "text/html"
//content: any utf-8 data, need html-encode if content_type is "text/html"
//content_length: can be -1 if content is string
//respone_size: if not NULL, the size of respone will be writen to it
//returns malloc()ed respone, need free() by caller
char* format_http_respone(const char* status, const char* content_type, const void* content, int content_length, int* respone_size) {
	int totalsize, header_size;
	char* respone;
	if(content_length < 0)
		content_length = content ? strlen((char*)content) : 0;
	totalsize = strlen(status) + strlen(content_type) + content_length + 128;
	respone = (char*) malloc(totalsize);
	header_size = sprintf(respone,  "HTTP/1.1 %s\r\n"
									"Content-Type:%s;charset=utf-8\r\n"
									"Content-Length:%d\r\n\r\n",
						status, content_type, content_length);
	assert(header_size > 0);
	if(content) {
		memcpy(respone + header_size, content, content_length);
	}
	if(respone_size)
		*respone_size = header_size + content_length;
	return respone;
}

#if defined(WIN32)
	#define snprintf _snprintf
#endif

static void _404_not_found(uv_stream_t* client, const char* pathinfo) {
	char* respone;
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "<h1>404 Not Found</h1><p>%s</p>", pathinfo);
	respone = format_http_respone("404 Not Found", "text/html", buffer, -1, NULL);
	write_uv_data(client, respone, -1, 0, 1);
}

//invoked by tinyweb when a file is request. see tinyweb_on_request_get()
static void _on_send_file(uv_stream_t* client, const char* content_type, const char* file, const char* pathinfo) {
	int file_size, read_bytes, respone_size;
	unsigned char *file_data, *respone;
	FILE* fp = fopen(file, "rb");
	if(fp) {
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		file_data = (unsigned char*) malloc(file_size);
		read_bytes = fread(file_data, 1, file_size, fp);
		assert(read_bytes == file_size);
		fclose(fp);

		respone_size = 0;
		respone = format_http_respone("200 OK", content_type, file_data, file_size, &respone_size);
		free(file_data);
		write_uv_data(client, respone, respone_size, 0, 1);
	} else {
		_404_not_found(client, pathinfo);
	}
}

//only identify familar postfix currently
static const char* _get_content_type(const char* postfix) {
	if(strcmp(postfix, "html") == 0 || strcmp(postfix, "htm") == 0)
		return "text/html";
	else if(strcmp(postfix, "js") == 0)
		return "text/javascript";
	else if(strcmp(postfix, "css") == 0)
		return "text/css";
	else if(strcmp(postfix, "jpeg") == 0 || strcmp(postfix, "jpg") == 0)
		return "image/jpeg";
	else if(strcmp(postfix, "png") == 0)
		return "image/png";
	else if(strcmp(postfix, "gif") == 0)
		return "image/gif";
	else if(strcmp(postfix, "txt") == 0)
		return "text/plain";
	else
		return "application/octet-stream";
}

//invoked by tinyweb when GET request comes in
//please invoke write_uv_data() once and only once on every request, to send respone to client and close the connection.
//if not handle this request (by invoking write_uv_data()), you can close connection using tinyweb_close_client(client).
//pathinfo: "/" or "/book/view/1"
//query_stirng: the string after '?' in url, such as "id=0&value=123", maybe NULL or ""
static void tinyweb_on_request_get(uv_stream_t* client, const char* pathinfo, const char* query_stirng) {
	//request a file?
	char* postfix = strrchr(pathinfo, '.');
	if(postfix) {
		postfix++;
		if(_doc_root_path) {
			char file[1024];
			snprintf(file, sizeof(file), "%s%s", _doc_root_path, pathinfo);
			_on_send_file(client, _get_content_type(postfix), file, pathinfo);
			return;
		} else {
			_404_not_found(client, pathinfo);
			return;
		}
	}

	//request an action
	if(strcmp(pathinfo, "/") == 0) {
		char* respone = format_http_respone("200 OK", "text/html", "Welcome to tinyweb", -1, NULL);
		write_uv_data(client, respone, -1, 0, 1);
	} else if(strcmp(pathinfo, "/404") == 0) {
		char* respone = format_http_respone("404 Not Found", "text/html", "<h3>404 Page Not Found<h3>", -1, NULL);
		write_uv_data(client, respone, -1, 0, 1);
	} else {
		char* respone;
		char content[1024];
		snprintf(content, sizeof(content), "<h1>tinyweb</h1><p>pathinfo: %s</p><p>query stirng: %s</p>", pathinfo, query_stirng);
		respone = format_http_respone("200 OK", "text/html", content, -1, NULL);
		write_uv_data(client, respone, -1, 0, 1);
	}
}

static void on_uv_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	*buf = uv_buf_init(malloc(suggested_size), suggested_size);
}

static void on_uv_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
	if(nread > 0) {
		char* crln2;
		membuf_t* membuf = (membuf_t*) client->data; //see tinyweb_on_connection()
		assert(membuf);
		membuf_append_data(membuf, buf->base, nread);
		if((crln2 = strstr((const char*)membuf->data, "\r\n\r\n")) != NULL) {
			const char* request_header = membuf->data;
			*crln2 = '\0';
			printf("\n----Tinyweb client request: ----\n%s\n", request_header);
			//handle GET request
			if(request_header[0]=='G' && request_header[1]=='E' && request_header[2]=='T') {
				char *query_stirng, *end;
				const char* pathinfo = request_header + 3;
				while(isspace(*pathinfo)) pathinfo++;
				end = strchr(pathinfo, ' ');
				if(end) *end = '\0';
				query_stirng = strchr(pathinfo, '?'); //split pathinfo and query_stirng using '?'
				if(query_stirng) {
					*query_stirng = '\0';
					query_stirng++;
				}
				tinyweb_on_request_get(client, pathinfo, query_stirng);
				//write_uv_data -> after_uv_write -> uv_close, will close the client, so additional processing is not needed.
			} else {
				tinyweb_close_client(client);
			}
		}
	} else if(nread == -1) {
		tinyweb_close_client(client);
	}

	if(buf->base)
		free(buf->base);
}

static void tinyweb_on_connection(uv_stream_t* server, int status) {
	assert(server == (uv_stream_t*)&_server);
	if(status == 0) {
		uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
		client->data = malloc(sizeof(membuf_t));
		membuf_init((membuf_t*)client->data, 128);
		uv_tcp_init(_loop, client);
		uv_accept((uv_stream_t*)&_server, (uv_stream_t*)client);
		uv_read_start((uv_stream_t*)client, on_uv_alloc, on_uv_read);
		
		//write_uv_data((uv_stream_t*)client, http_respone, -1, 0);
		//close client after uv_write, and free it in after_uv_close()
	}
}
