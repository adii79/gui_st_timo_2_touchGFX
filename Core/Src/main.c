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

/* USER CODE BEGIN Includes */
#include "ili9488.h"
#include "xpt2046.h"
#include "ui_test.h"
#include "ui_gfx.h"
#include <stdio.h>
#include "ugfx.h"
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

/*
 * Keep handles to widgets that callbacks need to reference.
 * Declared here so both the setup code and callbacks can access them.
 */
static ugfx_slider_t *g_sliderR = NULL;   /* "Red"   slider  */
static ugfx_slider_t *g_sliderG = NULL;   /* "Green" slider  */
static ugfx_slider_t *g_sliderB = NULL;   /* "Blue"  slider  */
static ugfx_label_t  *g_label   = NULL;   /* status label    */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */

/* ══════════════════════════════════════════════════════════════════════════
   SLIDER CALLBACKS
   Called every time the user moves a slider.  value is already clamped.
   ══════════════════════════════════════════════════════════════════════════ */

int atest = 0;
int btest = 0;
int ctest = 0;


static void OnSliderR_Changed(int32_t value)
{
    atest = (int)value;
    snprintf(msg, sizeof(msg), "A:%3d", atest);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

static void OnSliderG_Changed(int32_t value)
{
    btest = (int)value;
    snprintf(msg, sizeof(msg), "B:%3d", btest);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

static void OnSliderB_Changed(int32_t value)
{
    ctest = (int)value;
    snprintf(msg, sizeof(msg), "C:%3d", ctest);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

/* ══════════════════════════════════════════════════════════════════════════
   BUTTON CALLBACKS
   Signature:  void MyCallback(ugfx_button_t *btn)
   The engine calls this on finger-DOWN with a pointer to the button.
   You may freely mutate:
     btn->label        — changes the text displayed on the button
     btn->col_idle     — idle background colour
     btn->col_press    — pressed background colour
     btn->col_text     — label text colour
   The engine redraws the button after this function returns.
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  "OK" button pressed.
 *         Resets all three sliders to 0 and updates the label.
 */
static void OnOkPressed(ugfx_button_t *btn)
{
    btn->label = "RESET!";

    atest = 0; btest = 0; ctest = 0;

    if (g_sliderR) UGFX_SliderSetValue(g_sliderR, 0);
    if (g_sliderG) UGFX_SliderSetValue(g_sliderG, 0);
    if (g_sliderB) UGFX_SliderSetValue(g_sliderB, 0);

    if (g_label) UGFX_LabelSetText(g_label, "Reset OK");
}

/**
 * @brief  "TEST" button pressed.
 *         Sets all sliders to mid-range and changes the button colour.
 */
static void OnTestPressed(ugfx_button_t *btn)
{
    btn->label    = "MID!";
    btn->col_idle = 0xF800u;
    btn->col_text = 0x0000u;

    atest = 100; btest = 100; ctest = 100;

    if (g_sliderR) UGFX_SliderSetValue(g_sliderR, 100);
    if (g_sliderG) UGFX_SliderSetValue(g_sliderG, 100);
    if (g_sliderB) UGFX_SliderSetValue(g_sliderB, 100);

    if (g_label) UGFX_LabelSetText(g_label, "Mid set");
}

int test = 0;
static void OnTestPresseda(ugfx_button_t *btn)
{
//    btn->label    = "MID!";
//    btn->col_idle = 0xF800u;
//    btn->col_text = 0x0000u;
//
//    atest = 100; btest = 100; ctest = 100;
//
//    if (g_sliderR) UGFX_SliderSetValue(g_sliderR, 100);
//    if (g_sliderG) UGFX_SliderSetValue(g_sliderG, 100);
//    if (g_sliderB) UGFX_SliderSetValue(g_sliderB, 100);
//
//    if (g_label) UGFX_LabelSetText(g_label, "Mid set");
	test++;

}

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

    /* MCU Configuration ---------------------------------------------------- */
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    /* Deassert both CS lines before anything else */
    HAL_GPIO_WritePin(DISPL_CS_GPIO_Port, DISPL_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    /* Bring up display and touch controller */
    ILI9488_Init(&hspi1);
    ILI9488_SetOrientation(ILI9488_ORIENT_LANDSCAPE);
    XPT2046_Init(&hspi1);

    /* ── µGFX setup ──────────────────────────────────────────────────────── */
    UGFX_Init();
    UGFX_Begin();

    /* ── Slider: Red (vertical, x=240) ──────────────────────────────────── */
    ugfx_slider_builder_t *sb = Slider(0, 255, 100);
    sb->frame    (sb, 30, 150);
    sb->origin   (sb, 40, 40);
    sb->direction(sb, UGFX_VERTICAL);
    sb->onChanged(sb, OnSliderR_Changed);   /* wire callback */
    g_sliderR = sb->build(sb);

    /* ── Slider: Green (vertical, x=300) ─────────────────────────────────── */
    ugfx_slider_builder_t *sb1 = Slider(0, 255, 80);
    sb1->frame    (sb1, 300, 20);
    sb1->origin   (sb1, 100, 40);
    sb1->direction(sb1, UGFX_HORIZONTAL);
    sb1->onChanged(sb1, OnSliderG_Changed);
    g_sliderG = sb1->build(sb1);

    /* ── Slider: Blue (vertical, x=360) ──────────────────────────────────── */
    ugfx_slider_builder_t *sb2 = Slider(0, 255, 160);
    sb2->frame    (sb2, 300, 20);
    sb2->origin   (sb2, 100, 150);
    sb2->direction(sb2, UGFX_HORIZONTAL);
    sb2->onChanged(sb2, OnSliderB_Changed);
    g_sliderB = sb2->build(sb2);

    /* ── Button: OK ──────────────────────────────────────────────────────── */
    ugfx_button_builder_t *bb = Button("OK");
    bb->frame (bb, 80, 50);
    bb->origin(bb, 300, 270);
    bb->onTap (bb, OnOkPressed);            /* wire callback */
    bb->build (bb);

    /* ── Button: TEST ────────────────────────────────────────────────────── */
    ugfx_button_builder_t *bbb = Button("TEST");
    bbb->frame (bbb, 80, 50);
    bbb->origin(bbb, 350, 270);
    bbb->onTap (bbb, OnTestPressed);        /* wire callback */
    bbb->build (bbb);

    ugfx_button_builder_t *bbb1 = Button("a++");
        bbb1->frame (bbb1, 50, 50);
        bbb1->origin(bbb1, 430, 270);
        bbb1->onTap (bbb1, OnTestPresseda);        /* wire callback */
        bbb1->build (bbb1);

    /* ── Label: status line ──────────────────────────────────────────────── */
    ugfx_label_builder_t *lb = Label("Ready");
    lb->origin(lb, 30, 280);
    lb->size  (lb, 2);
    g_label = lb->build(lb);

    /* Draw everything for the first time */
    UGFX_Commit();

    /* USER CODE END 2 */

    /* Infinite loop -------------------------------------------------------- */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */

        UGFX_Poll();

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

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 180;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief SPI1 Initialization Function
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    /* USER CODE BEGIN SPI1_Init 0 */
    /* USER CODE END SPI1_Init 0 */

    /* USER CODE BEGIN SPI1_Init 1 */
    /* USER CODE END SPI1_Init 1 */

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }

    /* USER CODE BEGIN SPI1_Init 2 */
    /* USER CODE END SPI1_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, DISPL_LED_Pin | DISPL_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, DISPL_RST_Pin | DISPL_CS_Pin | TOUCH_CS_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = DISPL_LED_Pin | DISPL_DC_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = DISPL_RST_Pin | DISPL_CS_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = TOUCH_CS_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TOUCH_CS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = TOUCH_INT_Pin;
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

/**
  * @brief  Error handler.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq();
    while (1) { }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
