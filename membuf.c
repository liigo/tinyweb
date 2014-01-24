#include "membuf.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

void membuf_init(membuf_t* buf, unsigned int initial_buffer_size) {
	memset(buf, 0, sizeof(membuf_t));
	if(initial_buffer_size <= MEMBUF_INLINE_CAPACITY) {
		buf->data = buf->inline_buffer;
		buf->buffer_size = MEMBUF_INLINE_CAPACITY;
	} else {
		buf->data = malloc(initial_buffer_size);
		assert(buf->data);
		buf->buffer_size = initial_buffer_size;
	}
}

void membuf_uninit(membuf_t* buf) {
	assert(buf->data);
	if(buf->data != buf->inline_buffer) {
		free(buf->data);
	}
	memset(buf, 0, sizeof(membuf_t));
}

void membuf_ensure_new_size(membuf_t* buf, unsigned int new_size) {
	if(new_size > buf->buffer_size - buf->size) {
		unsigned int new_data_size = buf->size + new_size;
		//calculate new buffer size
		unsigned int new_buffer_size = buf->buffer_size * 2;
		while(new_buffer_size < new_data_size)
			new_buffer_size *= 2;
		assert(new_buffer_size > MEMBUF_INLINE_CAPACITY);

		// malloc/realloc new buffer and 
		if(buf->data == buf->inline_buffer) {
			buf->data = malloc(new_buffer_size);
			memcpy(buf->data, buf->inline_buffer, buf->size);
			memset(buf->inline_buffer, 0, MEMBUF_INLINE_CAPACITY);
		} else {
			buf->data = realloc(buf->data, new_buffer_size);
		}
		buf->buffer_size = new_buffer_size;
		memset(buf->data + buf->size, 0, new_buffer_size - buf->size);
	}
}

unsigned int membuf_append_data(membuf_t* buf, void* data, unsigned int size) {
	assert(data && size > 0);
	membuf_ensure_new_size(buf, size);
	memmove(buf->data + buf->size, data, size);
	buf->size += size;
	return (buf->size - size);
}

unsigned int membuf_append_zeros(membuf_t* buf, unsigned int size) {
	membuf_ensure_new_size(buf, size);
	//after exchange(), it maybe leaves non-zeroed data in unused buffer
	memset(buf->data + buf->size, 0, size);
	buf->size += size;
	return (buf->size - size);
}

unsigned int membuf_append_text(membuf_t* buf, const char* str, unsigned int len) {
	if(str && (len == (unsigned int)(-1)))
		len = strlen(str);
	return membuf_append_data(buf, (void*)str, len);
}

void membuf_exchange(membuf_t* buf1, membuf_t* buf2) {
	unsigned int tmp_size, tmp_buffer_size;
	assert(buf1 && buf2);

	//exchange data
	if(buf1->data == buf1->inline_buffer) {
		if(buf2->data == buf2->inline_buffer) {
			//both use inline buffer
			if(buf1->size <= buf2->size) {
				// #1
				unsigned char tmp_buffer[MEMBUF_INLINE_CAPACITY];
				memcpy(tmp_buffer, buf1->inline_buffer, buf1->size);
				memcpy(buf1->inline_buffer, buf2->inline_buffer, buf2->size);
				memcpy(buf2->inline_buffer, tmp_buffer, buf1->size);
			} else {
				membuf_exchange(buf2, buf1); //goto #1
				return;
			}
		} else {
			// #2 
			//buf1 uses inline buffer, buf2 not
			buf1->data = buf2->data; buf2->data = buf2->inline_buffer;
			memcpy(buf2->inline_buffer, buf1->inline_buffer, buf1->size);
		}
	} else {
		if(buf2->data != buf2->inline_buffer) {
			//both not use inline buffer
			unsigned char* tmp_data = buf1->data;
			buf1->data = buf2->data; buf2->data = tmp_data;
		} else {
			//buf2 uses inline buffer, buf1 not
			membuf_exchange(buf2, buf1); //goto #2
			return;
		}
	}

	//exchange size and buffer_size
	tmp_size = buf1->size;
	tmp_buffer_size = buf1->buffer_size;
	buf1->size = buf2->size; buf2->size = tmp_size;
	buf1->buffer_size = buf2->buffer_size; buf2->buffer_size = tmp_buffer_size;
}
