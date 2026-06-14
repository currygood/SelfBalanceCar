#include "Ult_Avoid.h"
#include <stdbool.h>
#include <stdlib.h>

static uint16_t dist_buffer[FILTER_WINDOW_SIZE] = {0};
static uint8_t  buffer_index = 0;
static uint16_t last_final_dist = 100; // 初始假设在1米处

// 辅助函数：冒泡排序取中值
static void sort(uint16_t a[], uint8_t n) {
    for (uint8_t i = 0; i < n - 1; i++) {
        for (uint8_t j = 0; j < n - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                uint16_t temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }
}

uint16_t Ult_GetFilteredDistance(uint16_t raw_len)
{
    // --- 策略1：剔除超时错误值 ---
    // 超声波在极近距离通常会返回最大值(如400cm+)，直接拦截
    if (raw_len > MAX_VALID_DISTANCE) {
        // 如果之前已经在近距离范围内，这次突然跳到最大，判定为“盲区错误”
        if (last_final_dist < NEAR_ZONE_THRESHOLD) {
            return 1; // 强制返回极小值，触发避障
        }
        raw_len = MAX_VALID_DISTANCE; 
    }

    // --- 策略2：中值滤波 (Median Filter) ---
    // 中值滤波对于剔除像 10, 12, 400, 11, 13 这种序列中的 400 非常有效
    dist_buffer[buffer_index] = raw_len;
    buffer_index = (buffer_index + 1) % FILTER_WINDOW_SIZE;

    uint16_t temp_buf[FILTER_WINDOW_SIZE];
    for(int i=0; i<FILTER_WINDOW_SIZE; i++) temp_buf[i] = dist_buffer[i];
    
    sort(temp_buf, FILTER_WINDOW_SIZE);
    uint16_t median = temp_buf[FILTER_WINDOW_SIZE / 2];

    // --- 策略3：动态跳变限制 ---
    // 如果测量值突然变化巨大（例如从10cm跳到100cm），且不是连续发生的，则进行限速
    // 这样可以防止单次采样错误导致的数值剧烈波动
    if (abs((int)median - (int)last_final_dist) > MAX_JUMP_STEP) {
        if (median > last_final_dist)
            median = last_final_dist + 5; // 缓慢增加
        else
            median = last_final_dist - 5; // 缓慢减小
    }

    last_final_dist = median;
    return median;
}

bool isNeedAvoid(uint16_t len)
{
    static int needAvoid_Count = 0;
    
    // 如果距离小于安全值且不为0（0通常是异常，例如传感器没有接）
    if(len < NEEDAVOID_LEN && len > 0)
        needAvoid_Count++;
    else
        needAvoid_Count = 0;
    
    if(needAvoid_Count >= NEEDAVOID_COUNT)
        return true;
    
    return false;
}