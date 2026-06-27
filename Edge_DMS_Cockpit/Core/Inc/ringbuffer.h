#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define RING_BUFFER_SIZE 1024
typedef struct {
	uint8_t buffer[RING_BUFFER_SIZE];
	uint16_t head;
	uint16_t tail;
}RingBuffer_t ;


uint16_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *out_data, uint16_t max_len);
void RingBuffer_Write(RingBuffer_t *rb, const uint8_t *data, uint16_t len);
