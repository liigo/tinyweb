#include <stdlib.h>
#include <uv.h>

#include "../tinyweb3.h"

int main()
{
	tinyweb_start(uv_default_loop(), "127.0.0.1", 8080, "D:\\Liigo\\github\\tinyweb\\test\\wwwroot");
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
