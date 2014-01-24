#ifndef __MEMBUF_H__
#define __MEMBUF_H__

// membuf is an auto-enlarging continuous in-memory buffer.
// it also has inline-buffer to use stack memory efficiently.
// by liigo, 2013-7-5.
// https://github.com/liigo/membuf

#include <stdlib.h>

#ifndef MEMBUF_INLINE_CAPACITY
	#define MEMBUF_INLINE_CAPACITY  512
#endif

typedef struct {
	unsigned char* data;
	unsigned int   size;
	unsigned int   buffer_size;
	unsigned char  inline_buffer[MEMBUF_INLINE_CAPACITY];
} membuf_t;

void membuf_init(membuf_t* buf, unsigned int initial_buffer_size);
void membuf_uninit(membuf_t* buf);

unsigned int membuf_append_data(membuf_t* buf, void* data, unsigned int size);
unsigned int membuf_append_zeros(membuf_t* buf, unsigned int size);
unsigned int membuf_append_text(membuf_t* buf, const char* str, unsigned int len);

static void* membuf_get_data(membuf_t* buf) { return (buf->size == 0 ? NULL : buf->data); }
static unsigned int membuf_get_size(membuf_t* buf) { return buf->size; }
static unsigned int membuf_empty(membuf_t* buf) { buf->size = 0; }
static unsigned int membuf_is_empty(membuf_t* buf) { return buf->size > 0; }

void membuf_ensure_new_size(membuf_t* buf, unsigned int new_size);
void membuf_exchange(membuf_t* buf1, membuf_t* buf2);


#if defined(_MSC_VER)
	#define MEMBUF_INLINE _inline
#else
	#define MEMBUF_INLINE static inline
#endif

MEMBUF_INLINE unsigned int membuf_append_byte(membuf_t* buf, unsigned char b) {
	return membuf_append_data(buf, &b, sizeof(b));
}
MEMBUF_INLINE unsigned int membuf_append_int(membuf_t* buf, int i) {
	return membuf_append_data(buf, &i, sizeof(i));
}
MEMBUF_INLINE unsigned int membuf_append_uint(membuf_t* buf, unsigned int ui) {
	return membuf_append_data(buf, &ui, sizeof(ui));
}
MEMBUF_INLINE unsigned int membuf_append_short(membuf_t* buf, short s) {
	return membuf_append_data(buf, &s, sizeof(s));
}
MEMBUF_INLINE unsigned int membuf_append_ushort(membuf_t* buf, short us) {
	return membuf_append_data(buf, &us, sizeof(us));
}
MEMBUF_INLINE unsigned int membuf_append_float(membuf_t* buf, float f) {
	return membuf_append_data(buf, &f, sizeof(f));
}
MEMBUF_INLINE unsigned int membuf_append_double(membuf_t* buf, double d) {
	return membuf_append_data(buf, &d, sizeof(d));
}


#endif //__MEMBUF_H__
