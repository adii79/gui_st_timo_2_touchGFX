/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "gradient.h"
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
 */
static ugfx_slider_t *g_sliderR = NULL;   /* Slider 1 — Rainbow / hue picker (VERTICAL)  */
static ugfx_slider_t *g_sliderG = NULL;   /* Slider 2 — Black → hue (HORIZONTAL)         */
static ugfx_slider_t *g_sliderB = NULL;   /* Slider 3 — Alpha → Blue (HORIZONTAL)        */
static ugfx_label_t  *g_label   = NULL;   /* status label                                */

/*
 * Declare the knob bitmap defined in ugfx.c so main.c can reference it.
 * The definition (KNOB_BMP_DATA[] + g_knob_bmp) lives in ugfx.c.
 */
extern const UI_Bitmap_t g_knob_bmp;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */

/* ══════════════════════════════════════════════════════════════════════════
   SLIDER CALLBACKS
   ══════════════════════════════════════════════════════════════════════════ */

int atest = 0;
int btest = 0;
int ctest = 0;

/**
 * @brief  Slider 1 — Rainbow hue picker (VERTICAL).
 *         Maps the 0-254 slider value → 0-360 hue and repaints ONLY the
 *         slider-2 pill track.
 */
static void OnSliderR_Changed(int32_t value)
{
    atest = (int)value;

    /* Map 0–254 slider range → 0–360 hue degrees */
    g_Slider2_Hue = (uint16_t)((value * 360) / 254);

    /* Repaint slider-2 gradient pill only — zero flicker */
    GRADIENT_RedrawSlider2Track();

    snprintf(msg, sizeof(msg), "H:%3u", (unsigned)g_Slider2_Hue);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

/**
 * @brief  Slider 2 — Black → dynamic hue (HORIZONTAL).
 */
static void OnSliderG_Changed(int32_t value)
{
    btest = (int)value;
    snprintf(msg, sizeof(msg), "B:%3d", btest);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

/**
 * @brief  Slider 3 — Alpha → Blue (HORIZONTAL).
 */
static void OnSliderB_Changed(int32_t value)
{
    ctest = (int)value;
    snprintf(msg, sizeof(msg), "C:%3d", ctest);
    if (g_label) UGFX_LabelSetText(g_label, msg);
}

/* ══════════════════════════════════════════════════════════════════════════
   BUTTON CALLBACKS
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  RESET — resets all three sliders to 0 and restores hue.
 */
static void OnOkPressed(ugfx_button_t *btn)
{
    btn->label = "RESET!";

    atest = 0; btest = 0; ctest = 0;

    if (g_sliderR) UGFX_SliderSetValue(g_sliderR, 0);
    if (g_sliderG) UGFX_SliderSetValue(g_sliderG, 0);
    if (g_sliderB) UGFX_SliderSetValue(g_sliderB, 0);

    g_Slider2_Hue = 0u;
    GRADIENT_RedrawSlider2Track();

    if (g_label) UGFX_LabelSetText(g_label, "Reset OK");
}

/**
 * @brief  MID — sets all sliders to mid-range (100).
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

    g_Slider2_Hue = (uint16_t)((100 * 360) / 255);
    GRADIENT_RedrawSlider2Track();

    if (g_label) UGFX_LabelSetText(g_label, "Mid set");
}

int test = 0;
static void OnTestPresseda(ugfx_button_t *btn)
{
    (void)btn;
    test++;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    HAL_GPIO_WritePin(DISPL_CS_GPIO_Port, DISPL_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    ILI9488_Init(&hspi1);
    ILI9488_SetOrientation(ILI9488_ORIENT_LANDSCAPE);
    XPT2046_Init(&hspi1);

    /* ── µGFX setup ──────────────────────────────────────────────────────── */
    UGFX_Init();
    UGFX_Begin();

    /* ══════════════════════════════════════════════════════════════════════
       SLIDER 1 — Rainbow / Hue picker  (VERTICAL, left side)
       Gradient pill: full hue rainbow, top=red → bottom=magenta
       bg_redraw_fn: GRADIENT_RedrawSlider1Track restores pill after erase
       knobBitmap:   24×24 diamond bitmap, white tint
       ══════════════════════════════════════════════════════════════════════ */
//    ugfx_slider_builder_t *sb = Slider(0, 254, 100);
//    sb->frame    (sb, 30, 350);
//    sb->origin   (sb, 12.5, 60);
//    sb->direction(sb, UGFX_HORIZONTAL);    /* col_track / col_fill are invisible behind the gradient pill,
//       but set them transparent so if bg_redraw_fn is ever removed
//       the track at least blends quietly */
//    sb->colors   (sb, 0x0000u, 0x0000u, 0xFFFFu);
//    sb->bgRedraw (sb, GRADIENT_RedrawSlider1Track);
//    sb->knobBitmap(sb, &g_knob_bmp, 1u, 0xFFFFu);
////    sb->onChanged(sb, OnSliderR_Changed);
//    g_sliderR = sb->build(sb);
//
//    /* ══════════════════════════════════════════════════════════════════════
//       SLIDER 2 — Black → dynamic hue  (HORIZONTAL)
//       Gradient pill: black → dark hue → bright hue (driven by slider 1)
//       bg_redraw_fn: GRADIENT_RedrawSlider2Track restores pill after erase
//       knobBitmap:   24×24 diamond bitmap, white tint
//       ══════════════════════════════════════════════════════════════════════ */
//    ugfx_slider_builder_t *sb1 = Slider(0, 254, 80);
//    sb1->frame    (sb1, 360, 15);
//    sb1->origin   (sb1, 125, 60);
//    sb1->direction(sb1, UGFX_HORIZONTAL);
//    sb1->colors   (sb1, 0x0000u, 0x0000u, 0xFFFFu);
//    sb1->bgRedraw (sb1, GRADIENT_RedrawSlider2Track);
//    sb1->knobBitmap(sb1, &g_knob_bmp, 1u, 0xFFFFu);
//    sb1->onChanged(sb1, OnSliderG_Changed);
//    g_sliderG = sb1->build(sb1);
//
//    /* ══════════════════════════════════════════════════════════════════════
//       SLIDER 3 — Checkerboard → Blue  (HORIZONTAL)
//       Gradient pill: checkerboard → solid blue (alpha illusion)
//       bg_redraw_fn: GRADIENT_RedrawSlider3Track restores pill after erase
//       knobBitmap:   24×24 diamond bitmap, white tint
//       ══════════════════════════════════════════════════════════════════════ */
//    ugfx_slider_builder_t *sb2 = Slider(0, 254, 160);
//    sb2->frame    (sb2, 360, 15);
//    sb2->origin   (sb2, 125, 185);
//    sb2->direction(sb2, UGFX_HORIZONTAL);
//    sb2->colors   (sb2, 0x0000u, 0x0000u, 0xFFFFu);
//    sb2->bgRedraw (sb2, GRADIENT_RedrawSlider3Track);
//    sb2->knobBitmap(sb2, &g_knob_bmp, 1u, 0xFFFFu);
//    sb2->onChanged(sb2, OnSliderB_Changed);
//    g_sliderB = sb2->build(sb2);
//
//    /* ── Buttons ─────────────────────────────────────────────────────────── */
//    ugfx_button_builder_t *bb = Button("OK");
//    bb->frame (bb, 80, 50);
//    bb->origin(bb, 300, 270);
//    bb->onTap (bb, OnOkPressed);
//    bb->build (bb);
//
//    ugfx_button_builder_t *bbb = Button("TEST");
//    bbb->frame (bbb, 80, 50);
//    bbb->origin(bbb, 350, 270);
//    bbb->onTap (bbb, OnTestPressed);
//    bbb->build (bbb);
//
//    ugfx_button_builder_t *bbb1 = Button("a++");
//    bbb1->frame (bbb1, 50, 50);
//    bbb1->origin(bbb1, 430, 270);
//    bbb1->onTap (bbb1, OnTestPresseda);
//    bbb1->build (bbb1);
//
//    /* ── Label (uncomment to enable) ────────────────────────────────────── */
//    /* ugfx_label_builder_t *lb = Label("Ready");
//       lb->origin(lb, 30, 280);
//       lb->size  (lb, 2);
//       g_label = lb->build(lb); */
//
//    /* ── Initial draw sequence ───────────────────────────────────────────
//     *
//     *  ORDER MATTERS:
//     *    1. FillScreen     — blank slate
//     *    2. DrawStaticSliderImage — paints background gradient + all three
//     *                        gradient pills (rainbow, black→hue, checker→blue)
//     *    3. UGFX_Commit    — draws buttons and slider knobs ON TOP of the pills
//     *
//     *  After this point every UGFX_SliderDraw() call will:
//     *    a. Erase only the knob bounding square
//     *    b. Call bg_redraw_fn() to restore the pill underneath
//     *    c. Draw the bitmap knob on top
//     *  producing zero ghost pixels and a gradient always visible behind knobs.
//     ─────────────────────────────────────────────────────────────────────── */
//
//    /* Boot hue matches the initial value of slider 1 (100 out of 254) */
//    g_Slider2_Hue = (uint16_t)((100 * 360) / 254);
//
//    ILI9488_FillScreen(0x0000u);
//
//    DrawStaticSliderImage();   /* background gradient + all three pills     */
//
//    UGFX_Commit();             /* buttons + knobs on top                    */
//    GRADIENT_RedrawSlider1Track();
//    GRADIENT_RedrawSlider2Track();
//    GRADIENT_RedrawSlider3Track();
//    /* USER CODE END 2 */




        /* ══════════════════════════════════════════════════════════════════════
           SLIDER 1 — Rainbow / Hue picker  (HORIZONTAL, top band)
           origin(100, 35), frame(360, 60)
           Maps 0-254 → hue 0-360, drives slider 2 gradient colour
//           ══════════════════════════════════════════════════════════════════════ */
//        ugfx_slider_builder_t *sb = Slider(0, 254, 100);
//        sb->frame    (sb, 360, 60);
//        sb->origin   (sb, 100, 35);
//        sb->direction(sb, UGFX_HORIZONTAL);
////        sb->colors   (sb, 0x0000u, 0x0000u, 0x0000u);
//        sb->bgRedraw (sb, GRADIENT_RedrawSlider1Track);
//        sb->knobBitmap(sb, &g_knob_bmp, 1u, 0xFFFFu);
//        sb->onChanged(sb, OnSliderR_Changed);
//        g_sliderR = sb->build(sb);
//
//        /* ══════════════════════════════════════════════════════════════════════
//           SLIDER 2 — Black → dynamic hue  (HORIZONTAL, middle band)
//           origin(100, 140), frame(360, 60)
//           ══════════════════════════════════════════════════════════════════════ */
//        ugfx_slider_builder_t *sb1 = Slider(0, 254, 80);
//        sb1->frame    (sb1, 360, 60);
//        sb1->origin   (sb1, 100, 140);
//        sb1->direction(sb1, UGFX_HORIZONTAL);
//        sb1->colors   (sb1, 0x0000u, 0x0000u, 0xFFFFu);
//        sb1->bgRedraw (sb1, GRADIENT_RedrawSlider2Track);
//        sb1->knobBitmap(sb1, &g_knob_bmp, 1u, 0xFFFFu);
//        sb1->onChanged(sb1, OnSliderG_Changed);
//        g_sliderG = sb1->build(sb1);
//
//        /* ══════════════════════════════════════════════════════════════════════
//           SLIDER 3 — Alpha → Blue  (VERTICAL, left column)
//           origin(25, 35), frame(60, 250)
//           ══════════════════════════════════════════════════════════════════════ */
//        ugfx_slider_builder_t *sb2 = Slider(0, 254, 160);
//        sb2->frame    (sb2, 60, 250);
//        sb2->origin   (sb2, 25, 35);
//        sb2->direction(sb2, UGFX_VERTICAL);
//        sb2->colors   (sb2, 0x0000u, 0x0000u, 0xFFFFu);
//        sb2->bgRedraw (sb2, GRADIENT_RedrawSlider3Track);
//        sb2->knobBitmap(sb2, &g_knob_bmp, 1u, 0xFFFFu);
//        sb2->onChanged(sb2, OnSliderB_Changed);
//        g_sliderB = sb2->build(sb2);
//
//        /* ── Boot hue matches slider 1 initial value (100 out of 254) ───── */
//        g_Slider2_Hue = (uint16_t)((100 * 360) / 254);
//
//        ugfx_button_builder_t *bbb1 = Button("a++");
//           bbb1->frame (bbb1, 50, 50);
//           bbb1->origin(bbb1, 430, 270);
//           bbb1->onTap (bbb1, OnTestPresseda);
//           bbb1->build (bbb1);
//
//        ILI9488_FillScreen(0x0000u);
//        DrawStaticSliderImage();
//        UGFX_Commit();


    ugfx_slider_builder_t *sb = Slider(0, 254, 100);
        sb->frame    (sb, 370, 60);
        sb->origin   (sb, 95, 35);
        sb->direction(sb, UGFX_HORIZONTAL);
        sb->colors   (sb, UGFX_COL_TRANSPARENT, UGFX_COL_TRANSPARENT, 0xFFFFu);
        sb->bgRedraw (sb, GRADIENT_RedrawSlider1Track);
        sb->knobBitmap(sb, &g_knob_bmp, 1u, 0xFFFFu);
        sb->onChanged(sb, OnSliderR_Changed);
        g_sliderR = sb->build(sb);

        /* ══════════════════════════════════════════════════════════════════════
           SLIDER 2 — Black → dynamic hue  (HORIZONTAL, middle band)
           Pill: origin(100,140), frame(360,60) — matches gradient.c S2_*
           ══════════════════════════════════════════════════════════════════════ */
        ugfx_slider_builder_t *sb1 = Slider(0, 254, 80);
        sb1->frame    (sb1, 360, 60);
        sb1->origin   (sb1, 100, 140);
        sb1->direction(sb1, UGFX_HORIZONTAL);
        sb1->colors   (sb1, UGFX_COL_TRANSPARENT, UGFX_COL_TRANSPARENT, 0xFFFFu);
        sb1->bgRedraw (sb1, GRADIENT_RedrawSlider2Track);
        sb1->knobBitmap(sb1, &g_knob_bmp, 1u, 0xFFFFu);
        sb1->onChanged(sb1, OnSliderG_Changed);
        g_sliderG = sb1->build(sb1);

        /* ══════════════════════════════════════════════════════════════════════
           SLIDER 3 — Alpha → Blue  (VERTICAL, left column)
           Pill: origin(25,35), frame(60,250) — matches gradient.c S3_*
           ══════════════════════════════════════════════════════════════════════ */
        ugfx_slider_builder_t *sb2 = Slider(0, 254, 160);
        sb2->frame    (sb2, 60, 250);
        sb2->origin   (sb2, 25, 35);
        sb2->direction(sb2, UGFX_VERTICAL);
        sb2->colors   (sb2, UGFX_COL_TRANSPARENT, UGFX_COL_TRANSPARENT, 0xFFFFu);
        sb2->bgRedraw (sb2, GRADIENT_RedrawSlider3Track);
        sb2->knobBitmap(sb2, &g_knob_bmp, 1u, 0xFFFFu);
        sb2->onChanged(sb2, OnSliderB_Changed);
        g_sliderB = sb2->build(sb2);

        /* ── Button ──────────────────────────────────────────────────────────── */
        ugfx_button_builder_t *bbb1 = Button("a++");
        bbb1->frame (bbb1, 50, 50);
        bbb1->origin(bbb1, 430, 270);
        bbb1->onTap (bbb1, OnTestPresseda);
        bbb1->build (bbb1);

        /* ── Boot hue matches slider 1 initial value (100/254) ──────────────── */
        g_Slider2_Hue = (uint16_t)((100 * 360) / 254);

        ILI9488_FillScreen(0x0000u);
        DrawStaticSliderImage();   /* background gradient + all three pills      */
        UGFX_Commit();


    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */

        UGFX_Poll();

        /* USER CODE END 3 */
    }
}

/**
  * @brief System Clock Configuration
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) { Error_Handler(); }
}

/**
  * @brief SPI1 Initialization
  */
static void MX_SPI1_Init(void)
{
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
    if (HAL_SPI_Init(&hspi1) != HAL_OK) { Error_Handler(); }
}

/**
  * @brief GPIO Initialization
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

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
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TOUCH_INT_Pin)
        XPT2046_IRQHandler();
}
/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
