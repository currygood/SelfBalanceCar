#include "Ultrasonicwave_Comm.h"
#include "DWT.h"
#include "main.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "Shared.h"
#include "event_groups.h"

EventGroupHandle_t UltComm_System_EventGroup = NULL;


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

void Ult_TrigGetDistance(EventGroupHandle_t eventgoup)
{
    Dis_Last_TimeTick = Dis_TimeTick_Get =  0;   // 新增
    UltComm_System_EventGroup = eventgoup;
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
            //计算差值的时候处理一下重装   例65535到0   第二次1-第一次65530   这是负数
            /*
            无符号数不用处理  
            举例说明（16 位简化版，便于理解）
            假设使用 uint16_t（0~65535），last = 65530，now = 1。

            数学上的真值：1 - 65530 = -65529

            C 语言中 uint16_t 的无符号减法：
            (1 - 65530) mod 65536 = -65529 mod 65536 = 7

            结果 7 正是从 65530 走到 1 实际经过的步数：

            不要直接用它做带符号判断，因为无符号数没有负数概念。   要处理方向就需要有符号数，然后用上面提到的相加相减处理
            */
            // 下降沿，计算差值并发送通知
            Dis_TimeTick_Get = DWT_Get_Cnt()-Dis_Last_TimeTick;
            // 任务通知
            /*
            xHigherPriorityTaskWoken 的作用
            pdFALSE（=0）：表示中断里没有唤醒任何比当前任务更高优先级的任务，中断返回后不会切换任务，CPU 继续执行被打断的那个任务。
            pdTRUE（≠0）：表示中断里唤醒了一个更高优先级的任务，中断返回时会通过 portYIELD_FROM_ISR(xHigherPriorityTaskWoken) 立刻切换到那个高优先级任务。
            */
            if(UltComm_System_EventGroup)
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xEventGroupSetBitsFromISR(UltComm_System_EventGroup, EVENT_GPIO_ECHO, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}