#include "Shared.h"

EventGroupHandle_t SystemEventGroup=NULL;

void Shared_Init()
{
    SystemEventGroup = xEventGroupCreate();
    configASSERT(SystemEventGroup != NULL);  // 检查是否创建成功
}

EventGroupHandle_t Shared_Get_EventGroupHandler()
{
    return SystemEventGroup;
}