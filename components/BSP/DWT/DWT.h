#ifndef __DWT_H
#define __DWT_H

#include "main.h"
#include <stdint.h>

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
// DWT初始化
uint8_t DWT_Init();

// DWT获取当前嘀嗒时间
uint64_t DWT_Get_Cnt();

#endif
