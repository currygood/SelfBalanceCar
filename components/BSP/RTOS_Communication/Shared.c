#include "Shared.h"

EventGroupHandle_t SystemEventGroup=NULL;

/**
 * @note 对外API接口
*/

/**
 * @brief 初始化共享资源
*/
void Shared_Init()
{
    SystemEventGroup = xEventGroupCreate();
    configASSERT(SystemEventGroup != NULL);  // 检查是否创建成功
}

/**
 * @brief 获取事件组句柄
 * @return EventGroupHandle_t 事件组句柄
*/
EventGroupHandle_t Shared_Get_EventGroupHandler()
{
    return SystemEventGroup;
}