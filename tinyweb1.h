#ifndef __TINYWEB_H__
#define __TINYWEB_H__

#include <uv.h>

void tinyweb_start(uv_loop_t* loop, const char* ip, int port);

#endif //__TINYWEB_H__
