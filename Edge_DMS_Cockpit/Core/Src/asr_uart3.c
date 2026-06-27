#include "asr_uart3.h"
#include <string.h>
#include "usart.h"
#include "main.h"
#include "bsp_stepper.h"
#include "sensor_data.h"
#include "cmsis_os.h"

#define ASR_RX_BUF_SIZE  32
static uint8_t  asr_rx_buf[ASR_RX_BUF_SIZE];
static uint16_t asr_rx_idx = 0;
 uint8_t  asr_rx_byte;
static volatile uint8_t asr_cmd_ready = 0;

void ASR_UART3_InitReceive(void)
{
    asr_rx_idx = 0;
    asr_cmd_ready = 0;
    memset(asr_rx_buf, 0, ASR_RX_BUF_SIZE);
    HAL_UART_Receive_IT(&huart3, &asr_rx_byte, 1);
}

void ASR_UART3_IRQHandler_Callback(void)
{
    if (asr_rx_idx < ASR_RX_BUF_SIZE - 1)
    {
        asr_rx_buf[asr_rx_idx++] = asr_rx_byte;
        
        if (asr_rx_idx >= 2)
        {
            asr_cmd_ready = 1;
        }
    }
    else
    {
        asr_rx_idx = 0;
        memset(asr_rx_buf, 0, ASR_RX_BUF_SIZE);
    }
    
    HAL_UART_Receive_IT(&huart3, &asr_rx_byte, 1);
}

void ASR_ProcessCommand(uint8_t cmd_high, uint8_t cmd_low)
{
    
    
    switch(cmd_high)
    {
        case 0x00:
            switch(cmd_low)
            {
                case 0x11:
                    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
               
                    break;
                case 0x10:
                    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
                    
                    break;
                case 0x21:
                    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
           
                    break;
                case 0x20:
                    HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
                   
                    break;
                case 0x31:
                    BSP_Stepper_SetWindowPosition(100);
                  
                    break;
                case 0x30:
                    BSP_Stepper_SetWindowPosition(0);
                   
                    break;
                case 0x41:
                    HAL_GPIO_WritePin(RLY2_GPIO_Port, RLY2_Pin, GPIO_PIN_SET);
                    
                    break;
                case 0x51:
                    HAL_GPIO_WritePin(RLY1_GPIO_Port, RLY1_Pin, GPIO_PIN_SET);
                    
                    break;
                case 0x60:
                    HAL_GPIO_WritePin(RLY2_GPIO_Port, RLY2_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(RLY1_GPIO_Port, RLY1_Pin, GPIO_PIN_RESET);
                    
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    

}

void ASR_ProcessRxData(void)
{
    if (asr_cmd_ready)
    {
        if (asr_rx_idx >= 2)
        {
            ASR_ProcessCommand(asr_rx_buf[0], asr_rx_buf[1]);
        }
        asr_rx_idx = 0;
        asr_cmd_ready = 0;
        memset(asr_rx_buf, 0, ASR_RX_BUF_SIZE);
    }
}