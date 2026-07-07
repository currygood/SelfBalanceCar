/*
OLED显示模块
*/

#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include "OLED_Data.h"
#include "main.h"
#include "I2C_Bus.h"

/*
    可供外部使用的宏定义
*/
/*参数宏定义*********************/

#define OLED_ADDRESS        0x3C
#define OLED_WRITE_DATA_ADD     0x40
#define OLED_WRITE_COMMAND_ADD  0x00

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度   也是不使用枚举的原因*/
#define OLED_8X16				8
#define OLED_6X8				6
#define OLED_12X24				12

/*IsFilled参数数值*/
#define OLED_UNFILLED			0
#define OLED_FILLED				1

/*********************参数宏定义*/

/*
    可供外部使用的结构体等抽象数据类型
*/

/*
    可供外部使用的extern变量
*/

/*
    对外暴露的API接口
*/
/*初始化函数*/
// OLED初始化
void OLED_Init(void);

/*更新函数*/
// 将OLED显存数组更新到OLED屏幕
void OLED_Update(void);
// OLED分区域更新
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
// OLED分页异步更新（RTOS专用）
void OLED_Update_RTOS(void);

/*显存控制函数*/
// 将OLED显存数组全部清零
void OLED_Clear(void);
// 将OLED显存数组部分清零
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
// 将OLED显存数组全部取反
void OLED_Reverse(void);
// 将OLED显存数组部分取反
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显示函数*/
// OLED显示一个字符
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
// OLED显示字符串
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
// OLED显示数字（十进制，正整数）
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
// OLED显示有符号数字（十进制，整数）
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
// OLED显示十六进制数字
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
// OLED显示二进制数字
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
// OLED显示浮点数字
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
// OLED显示图像
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
// OLED使用printf函数打印格式化字符串
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/*绘图函数*/
// OLED在指定位置画一个点
void OLED_DrawPoint(int16_t X, int16_t Y);
// OLED获取指定位置点的值
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
// OLED画线
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
// OLED矩形
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
// OLED三角形
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
// OLED圆形
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
// OLED椭圆
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
// OLED圆弧
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/*********************函数声明*/

#endif