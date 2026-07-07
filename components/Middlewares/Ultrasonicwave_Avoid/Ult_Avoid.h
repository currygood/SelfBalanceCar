/* Ult_Avoid.h */
#ifndef __ULT_AVOID_H
#define __ULT_AVOID_H

#include "main.h"
#include "Ultrasonicwave_Comm.h"
#include <stdint.h>
#include <stdbool.h>

/*
    可供外部使用的宏定义
*/
#define NEEDAVOID_LEN 10     // 最小安全距离 (cm)
#define NEEDAVOID_COUNT 2   // 连续多少次触发了最小安全距离就提醒

// 滤波
#define FILTER_WINDOW_SIZE 5
#define MAX_VALID_DISTANCE 400
#define MAX_JUMP_STEP 60      // 两次测量之间允许的最大跳变（厘米）
#define NEAR_ZONE_THRESHOLD 15 // 进入"危险近距离"的阈值

/*
    可供外部使用的结构体等抽象数据类型
*/

/*
    可供外部使用的extern变量
*/

/*
    对外暴露的API接口
*/
// 获取滤波后的距离值
uint16_t Ult_GetFilteredDistance(uint16_t raw_len);

// 判断是否需要避障
bool isNeedAvoid(uint16_t len);

#endif