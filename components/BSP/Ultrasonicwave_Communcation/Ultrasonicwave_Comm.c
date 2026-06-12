#include "Ultrasonicwave_Comm.h"
#include "DWT.h"
#include "main.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

TaskHandle_t UltComm_System_TaskHandler = NULL;

volatile static uint64_t Dis_Last_TimeTick=0;
volatile static uint64_t Dis_TimeTick_Get=0;


/* 
 * 延时函数：为了产生大于10us的高电平给超声波模块，开始测量距离
 * 72MHz 下，i=20 大约能产生 200KHz-300KHz 的波形
 */
static void Ult_Delay(void)
{
    uint32_t i = 360; 
    while(i--);
}

void Ult_TrigGetDistance(TaskHandle_t taskHandler)
{
    Dis_Last_TimeTick = Dis_TimeTick_Get =  0;   // 新增
    UltComm_System_TaskHandler = taskHandler;
    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, 1);
    Ult_Delay();
    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, 0);
}


uint16_t Ult_GetDistance(void)
{
    uint64_t diff_cycles = Dis_TimeTick_Get;
    float time_us = (float)diff_cycles * 1000000.0f / SystemCoreClock;
    float distance_cm = time_us * 0.0343f / 2.0f;   // 声速340m/s，单程
    return (uint16_t)distance_cm;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin==Echo_Pin)
    {
        if(HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin))
        {     // 上升沿，记录起始时间戳 开始计时
            Dis_Last_TimeTick = DWT_Get_Cnt();
        }
        else
        {
            // 下降沿，计算差值并发送通知
            Dis_TimeTick_Get = DWT_Get_Cnt()-Dis_Last_TimeTick;
            // 任务通知
            /*
            xHigherPriorityTaskWoken 的作用
            pdFALSE（=0）：表示中断里没有唤醒任何比当前任务更高优先级的任务，中断返回后不会切换任务，CPU 继续执行被打断的那个任务。
            pdTRUE（≠0）：表示中断里唤醒了一个更高优先级的任务，中断返回时会通过 portYIELD_FROM_ISR(xHigherPriorityTaskWoken) 立刻切换到那个高优先级任务。
            */
            if(UltComm_System_TaskHandler)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                vTaskNotifyGiveFromISR(UltComm_System_TaskHandler, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}