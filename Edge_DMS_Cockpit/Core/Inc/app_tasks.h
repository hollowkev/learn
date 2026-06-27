#ifndef __APP_TASKS_H__
#define __APP_TASKS_H__

// 声明这三个任务函数，供 RTOS 创建时使用
void Task_Parse(void *argument);
void Task_Report(void *argument);
void Task_SensorPoll(void *argument);
void Task_Sensor(void *argument);
void Task_Screen(void *argument);


#endif