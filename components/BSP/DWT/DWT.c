#include "DWT.h"
#include <stdint.h>

/*
    模块内部使用的宏定义
*/
//DWT相关寄存器定义
#define DWT_CONTROL (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT  (*(volatile uint32_t*)0xE0001004)
#define DEMCR       (*(volatile uint32_t*)0xE000EDFC)

//初始化DWT所需要的位
#define DEMCR_TRCENA        (1<<24)     //DEMCR的DWT使能位
#define DWT_CTRL_CYCCNTENA  (1<<0)      //DWT的CYCCNT使能位


/**
 * @note 对外API接口
*/

/**
 * @brief DWT初始化
 * @return uint8_t 初始化是否成功，1成功，0失败
*/
uint8_t DWT_Init()
{
    // 使能DWT模块的访问
    DEMCR |= DEMCR_TRCENA;

    // 将cycle counter清零并启用
    DWT_CYCCNT = 0;
    DWT_CONTROL |= DWT_CTRL_CYCCNTENA;

    // 检查DWT计数器是否已经启用
    if(DWT_CYCCNT)
        return 1;
    else
        return 0;
}

/**
 * @brief DWT获取当前嘀嗒时间
 * @return uint64_t 当前Cycle Count值
*/
uint64_t DWT_Get_Cnt()
{
    return DWT_CYCCNT;
}