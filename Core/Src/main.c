/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"
#include "MPU6050.h"
#include <stdint.h>
#include "Ultrasonicwave_Comm.h"
#include "DWT.h"
#include "Ult_Avoid.h"
#include "BLE_Serial.h"
#include "MTask.h"
#include "BLE.h"
#include "Moitor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  EventGroupHandle_t egHandler=NULL;
  // 任务通信模块
  Shared_Init();
  egHandler = Shared_Get_EventGroupHandler();
  // 初始化
  // DWT获取时间戳
  DWT_Init();

  // I2C设备
  I2C_Bus_Init(egHandler);
  OLED_Init();
  MPU6050_Init();

  // BLE模块
  BLE_Init(egHandler);

  // 电机模块
  Moitor_Init();

  // 任务创建  启动调度器
  xTaskCreate(Control_Task, "Control_Task",256, NULL, 10, NULL);
  xTaskCreate(System_Task, "System_Task",256, NULL, 2, NULL);
  vTaskStartScheduler();

  while(1)
  {
    
  }

  //BLE底层测试
  // uint8_t bytes[64];
  // BLE_Serial_Init(NULL);

  // while(1)
  // {
  //   if(BLE_Serial_isCanRead())
  //   {
  //     BLE_Serial_ReadByte(bytes);
  //     OLED_ShowString(0, 0, bytes, OLED_6X8);
  //     OLED_Update();
  //   }

  //   BLE_Serial_SendBytes("hello world\r\n",sizeof("hello world\r\n"),5);
  //   HAL_Delay(1000);
  // }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // bool isCanSendMPU6050=true;
  bool isAvoid=false;
  while (1)
  {
    isAvoid=false;
    // MPU6050_Data_Struct data;
    // // 等待总线空闲再发起 
    // if(isCanSendMPU6050) MPU6050_Request_Data();

    // if(MPU6050_Parse_Data(&data))
    // {
    //   isCanSendMPU6050=true;
    // }
    // else
    //   isCanSendMPU6050=false;

    // uint8_t tempStr[40];
    // snprintf((char*)tempStr, sizeof tempStr, "%d %d", data.Accx, data.AccY);
    // OLED_ClearArea(0,0,128,16);
    // OLED_ShowString(0, 0, (char*)tempStr, OLED_8X16);
    
    Ult_TrigGetDistance(NULL);
    HAL_Delay(20);  //目前测试用延时，后面会有任务通知
    uint16_t distance = Ult_GetDistance();
    uint16_t filtered_dist = Ult_GetFilteredDistance(distance);
    if(isNeedAvoid(filtered_dist))
      isAvoid=true;
    uint8_t tempStr3[20],tempStr2[20];
    snprintf(tempStr2, sizeof tempStr2, "dis:%d",distance);
    snprintf(tempStr3,sizeof tempStr3, "Is avoid:%s",isAvoid?"need":"not");
    OLED_ClearArea(0,16,128,32);
    OLED_ShowString(0, 16, (char*)tempStr2, OLED_8X16);
    OLED_ShowString(0, 32, (char*)tempStr3, OLED_8X16);

    OLED_Update();
    HAL_Delay(50);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
