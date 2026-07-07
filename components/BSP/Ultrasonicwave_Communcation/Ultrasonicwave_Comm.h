/*
超声波测距-通信底层
*/

#include "main.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/*
    可供外部使用的宏定义
*/

/*
    可供外部使用的结构体等抽象数据类型
*/

/*
    可供外部使用的extern变量
*/

/*
    对外暴露的API接口
*/
// 触发超声波测距
void Ult_TrigGetDistance(EventGroupHandle_t eventgoup);

// 获取超声波测量的距离值
uint16_t Ult_GetDistance(void);