#ifndef RING_BUFFER_H
#define RING_BUFFER_H
 
#include <stddef.h> 

typedef struct {
    char* buffer;
    size_t head;
    size_t tail;
    size_t size;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t* rb, char* buffer, size_t size);
void ring_buffer_write(ring_buffer_t* rb, const char* data, size_t len);
size_t ring_buffer_read(ring_buffer_t* rb, char* data, size_t len);

#endif
