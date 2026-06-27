#include "ringbuffer.h"


void RingBuffer_Write(RingBuffer_t *rb, const uint8_t *data, uint16_t len)
{
	for(int i=0;i<len;i++)
	{
		rb->buffer[rb->head]=data[i];
		rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
	}
	
}
uint16_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *out_data, uint16_t max_len)
{
	uint16_t count = 0;
	while(count<max_len&&rb->tail != rb->head)
	{
		out_data[count]=rb->buffer[rb->tail];
		rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
		count++;
	}
	return count;
}                                          