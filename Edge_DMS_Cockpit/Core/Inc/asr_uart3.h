#ifndef __ASR_UART3_H__
#define __ASR_UART3_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void ASR_UART3_InitReceive(void);
void ASR_UART3_IRQHandler_Callback(void);
void ASR_ProcessCommand(uint8_t cmd_high, uint8_t cmd_low);
void ASR_ProcessRxData(void);

#ifdef __cplusplus
}
#endif

#endif /* __ASR_UART3_H__ */