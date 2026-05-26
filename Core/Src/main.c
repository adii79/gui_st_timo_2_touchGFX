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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9488.h"
#include "xpt2046.h"
#include "ui_test.h"
#include <stdio.h>
/* USER CODE END Includes */
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
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
static char msg[64];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  /* Deassert both CS lines before anything else */
  HAL_GPIO_WritePin(DISPL_CS_GPIO_Port,  DISPL_CS_Pin,  GPIO_PIN_SET);
  HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port,  TOUCH_CS_Pin,  GPIO_PIN_SET);
  HAL_Delay(10);

  ILI9488_Init(&hspi1);
//  /*  ILI9488_ORIENT_LANDSCAPE = 0,   /* 480 wide, 320 tall  (default) */
//    ILI9488_ORIENT_PORTRAIT,        /* 320 wide, 480 tall             */
//    ILI9488_ORIENT_LANDSCAPE_FLIP,
//    ILI9488_ORIENT_PORTRAIT_FLIP
//	*/
  ILI9488_SetOrientation(ILI9488_ORIENT_LANDSCAPE);
  XPT2046_Init(&hspi1);


  UI_Test_Init();


//
//  /* TEST 1 — colour floods */
//  ILI9488_FillScreen(ILI9488_COLOR_RED);   HAL_Delay(600);
//  ILI9488_FillScreen(ILI9488_COLOR_GREEN); HAL_Delay(600);
//  ILI9488_FillScreen(ILI9488_COLOR_BLUE);  HAL_Delay(600);
//  ILI9488_FillScreen(ILI9488_COLOR_WHITE); HAL_Delay(600);
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK); HAL_Delay(300);
//
//  /* TEST 2 — 8 colour bars */
//  {
//      uint16_t colours[8] = {
//          ILI9488_COLOR_RED,   ILI9488_COLOR_GREEN,
//          ILI9488_COLOR_BLUE,  ILI9488_COLOR_YELLOW,
//          ILI9488_COLOR_CYAN,  ILI9488_COLOR_MAGENTA,
//          ILI9488_COLOR_WHITE, ILI9488_COLOR_BLACK
//      };
//      for (uint8_t i = 0; i < 8; i++)
//          ILI9488_FillRect((uint16_t)(i * 60), 0, 60, ILI9488_HEIGHT, colours[i]);
//      HAL_Delay(1500);
//  }
//
//  /* TEST 3 — rectangles */
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK);
//  ILI9488_FillRect( 10,  10, 200, 100, ILI9488_COLOR_RED);
//  ILI9488_FillRect( 10, 130, 200, 100, ILI9488_COLOR_BLUE);
//  ILI9488_FillRect(230,  10, 200, 100, ILI9488_COLOR_GREEN);
//  ILI9488_DrawRect(230, 130, 200, 100, ILI9488_COLOR_YELLOW);
//  ILI9488_DrawRect( 10,  10, 420, 300, ILI9488_COLOR_WHITE);
//  HAL_Delay(1500);
//
//  /* TEST 4 — lines */
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK);
//  ILI9488_DrawLine(0, 0, 479, 319, ILI9488_COLOR_WHITE);
//  ILI9488_DrawLine(479, 0, 0, 319, ILI9488_COLOR_CYAN);
//  ILI9488_DrawHLine(0, 160, 480, ILI9488_COLOR_YELLOW);
//  ILI9488_DrawVLine(240, 0, 320, ILI9488_COLOR_YELLOW);
//  for (uint16_t x = 0; x <= 479; x += 40) {
//      ILI9488_DrawLine(240, 160, x,   0, ILI9488_COLOR_RED);
//      ILI9488_DrawLine(240, 160, x, 319, ILI9488_COLOR_RED);
//  }
//  HAL_Delay(1500);
//
//  /* TEST 5 — circles */
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK);
//  ILI9488_FillCircle(120,  80, 60, ILI9488_COLOR_RED);
//  ILI9488_FillCircle(360,  80, 60, ILI9488_COLOR_GREEN);
//  ILI9488_FillCircle(120, 240, 60, ILI9488_COLOR_BLUE);
//  ILI9488_FillCircle(360, 240, 60, ILI9488_COLOR_YELLOW);
//  ILI9488_DrawCircle(240, 160, 80, ILI9488_COLOR_WHITE);
//  HAL_Delay(1500);
//
//  /* TEST 6 — text */
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK);
//  ILI9488_DrawString( 10,  10, "ILI9488 TEST",       ILI9488_COLOR_WHITE,   ILI9488_COLOR_BLACK, 3);
//  ILI9488_DrawString( 10,  60, "STM32F429",           ILI9488_COLOR_YELLOW,  ILI9488_COLOR_BLACK, 2);
//  ILI9488_DrawString( 10,  90, "480 x 320  SPI",      ILI9488_COLOR_CYAN,    ILI9488_COLOR_BLACK, 2);
//  ILI9488_DrawString( 10, 120, "RGB666 18-bit",        ILI9488_COLOR_GREEN,   ILI9488_COLOR_BLACK, 2);
//  ILI9488_DrawString( 10, 160, "KMRTM35018-SPI v2.0", ILI9488_COLOR_MAGENTA, ILI9488_COLOR_BLACK, 1);
//  HAL_Delay(2000);
//
//  /* TEST 7 — backlight blink */
//  ILI9488_FillScreen(ILI9488_COLOR_WHITE);
//  for (uint8_t i = 0; i < 4; i++) {
//      ILI9488_SetBacklight(false); HAL_Delay(300);
//      ILI9488_SetBacklight(true);  HAL_Delay(300);
//  }
//
//  /* TEST 8 — touch paint, prompt */
//  ILI9488_FillScreen(ILI9488_COLOR_BLACK);
  ILI9488_DrawString(50, 140, "Touch the screen!", ILI9488_COLOR_WHITE, ILI9488_COLOR_BLACK, 2);
  /* USER CODE END 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//	  XPT2046_Point_t pt;
//	      if (XPT2046_GetTouchPixel(&pt)) {
//	          static uint8_t cleared = 0;
//	          if (!cleared) { ILI9488_FillScreen(ILI9488_COLOR_BLACK); cleared = 1; }
//
//	          ILI9488_DrawHLine((pt.x > 4) ? pt.x-4 : 0, pt.y, 9, ILI9488_COLOR_RED);
//	          ILI9488_DrawVLine(pt.x, (pt.y > 4) ? pt.y-4 : 0, 9, ILI9488_COLOR_RED);
//	          ILI9488_DrawPixel(pt.x, pt.y, ILI9488_COLOR_WHITE);
//
//	          uint16_t rX=0, rY=0, rZ=0;
//	          XPT2046_GetTouchRaw(&rX, &rY, &rZ);
//	          snprintf(msg, sizeof(msg), "X:%3d Y:%3d  raw(%4d,%4d) Z:%4d  ",
//	                   pt.x, pt.y, rX, rY, rZ);
//	          ILI9488_DrawString(4, 4, msg, ILI9488_COLOR_YELLOW, ILI9488_COLOR_BLACK, 1);
//	          HAL_Delay(20);
//	      }

	  UI_Test_Poll();
	  /* USER CODE END 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DISPL_LED_Pin|DISPL_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, DISPL_RST_Pin|DISPL_CS_Pin|TOUCH_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DISPL_LED_Pin DISPL_DC_Pin */
  GPIO_InitStruct.Pin = DISPL_LED_Pin|DISPL_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : DISPL_RST_Pin DISPL_CS_Pin */
  GPIO_InitStruct.Pin = DISPL_RST_Pin|DISPL_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : TOUCH_CS_Pin */
  GPIO_InitStruct.Pin = TOUCH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TOUCH_INT_Pin */
  GPIO_InitStruct.Pin = TOUCH_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TOUCH_INT_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TOUCH_INT_Pin)
        XPT2046_IRQHandler();
}
/* USER CODE END 4 */
/* USER CODE END 4 */

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
