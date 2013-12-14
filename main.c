#include "tinyweb1.h"
#include <stdlib.h>
#include <uv.h>

int main() {
	tinyweb_start(uv_default_loop(), "127.0.0.1", 8080);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
