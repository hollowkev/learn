/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_tasks.h"
#include "radar.h"
#include <stdio.h>
#include "app_uart.h"
#include "asr_uart3.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for RadarTask */
osThreadId_t RadarTaskHandle;
const osThreadAttr_t RadarTask_attributes = {
  .name = "RadarTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Report */
osThreadId_t Task_ReportHandle;
const osThreadAttr_t Task_Report_attributes = {
  .name = "Task_Report",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Parse */
osThreadId_t Task_ParseHandle;
const osThreadAttr_t Task_Parse_attributes = {
  .name = "Task_Parse",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Task_Sensor */
osThreadId_t Task_SensorHandle;
const osThreadAttr_t Task_Sensor_attributes = {
  .name = "Task_Sensor",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for TASK_HMI */
osThreadId_t TASK_HMIHandle;
const osThreadAttr_t TASK_HMI_attributes = {
  .name = "TASK_HMI",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Voice */
osThreadId_t Task_VoiceHandle;
const osThreadAttr_t Task_Voice_attributes = {
  .name = "Task_Voice",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DataBusMutex */
osMutexId_t DataBusMutexHandle;
const osMutexAttr_t DataBusMutex_attributes = {
  .name = "DataBusMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartRadarTask(void *argument);
void StartReportTask(void *argument);
void StartParseTask(void *argument);
void StartSensorTask(void *argument);
void StartHmiTask(void *argument);
void StartVoiceTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of DataBusMutex */
  DataBusMutexHandle = osMutexNew(&DataBusMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of RadarTask */
  RadarTaskHandle = osThreadNew(StartRadarTask, NULL, &RadarTask_attributes);

  /* creation of Task_Report */
  Task_ReportHandle = osThreadNew(StartReportTask, NULL, &Task_Report_attributes);

  /* creation of Task_Parse */
  Task_ParseHandle = osThreadNew(StartParseTask, NULL, &Task_Parse_attributes);

  /* creation of Task_Sensor */
  Task_SensorHandle = osThreadNew(StartSensorTask, NULL, &Task_Sensor_attributes);

  /* creation of TASK_HMI */
  TASK_HMIHandle = osThreadNew(StartHmiTask, NULL, &TASK_HMI_attributes);

  /* creation of Task_Voice */
  Task_VoiceHandle = osThreadNew(StartVoiceTask, NULL, &Task_Voice_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartRadarTask */
/**
* @brief Function implementing the RadarTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRadarTask */
void StartRadarTask(void *argument)
{
  /* USER CODE BEGIN StartRadarTask */
	Radar_Init();
  Task_Radar(); 
  /* USER CODE END StartRadarTask */
}

/* USER CODE BEGIN Header_StartReportTask */
/**
* @brief Function implementing the Task_Report thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReportTask */
void StartReportTask(void *argument)
{
  /* USER CODE BEGIN StartReportTask */
  Task_Report(argument); // 
  /* USER CODE END StartReportTask */
}

/* USER CODE BEGIN Header_StartParseTask */
/**
* @brief Function implementing the Task_Parse thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartParseTask */
void StartParseTask(void *argument)
{
  /* USER CODE BEGIN StartParseTask */
  App_UART_Init();
  Task_Parse(argument);
  /* USER CODE END StartParseTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  /* Infinite loop */
   Task_Sensor(argument);
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartHmiTask */
/**
* @brief Function implementing the TASK_HMI thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHmiTask */
void StartHmiTask(void *argument)
{
  /* USER CODE BEGIN StartHmiTask */
  /* Infinite loop */
  Task_Screen(argument);
  /* USER CODE END StartHmiTask */
}

/* USER CODE BEGIN Header_StartVoiceTask */
/**
* @brief Function implementing the Task_Voice thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartVoiceTask */
void StartVoiceTask(void *argument)
{
  /* USER CODE BEGIN StartVoiceTask */
  ASR_UART3_InitReceive();
    
    for(;;)        
    {
        ASR_ProcessRxData();
        osDelay(100);
    }
  /* USER CODE END StartVoiceTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

