/*
    FreeRTOS任务模块
*/

#ifndef __MYTASK_H
#define __MYTASK_H

#include "main.h"

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
// 控制任务（平衡车主控）
void Control_Task(void *params);

// 系统任务（命令处理、显示等）
void System_Task(void *params);


#endif