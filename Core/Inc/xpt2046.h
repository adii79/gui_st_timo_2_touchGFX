/**
 ******************************************************************************
 * @file    xpt2046.h
 * @brief   XPT2046 Resistive Touch Controller Driver
 *          Shares SPI1 bus with ILI9488 display.
 *
 * Pin Mapping:
 *   SPI1  SCK  -> PB3  (shared with display)
 *   SPI1  MISO -> PB4  (shared with display)
 *   SPI1  MOSI -> PB5  (shared with display)
 *   TOUCH_CS   -> PF2  (active LOW – separate CS)
 *   TOUCH_INT  -> PF3  (active LOW interrupt, rising-edge trigger in CubeMX)
 *
 * Usage:
 *   1. Call XPT2046_Init() once after SPI1 is ready.
 *   2. Poll XPT2046_IsTouched() or rely on TOUCH_INT interrupt.
 *   3. Call XPT2046_GetTouchRaw() or XPT2046_GetTouchPixel() for coordinates.
 *   4. Run XPT2046_Calibrate() once on first use and store the result in flash.
 ******************************************************************************
 */

#ifndef XPT2046_H
#define XPT2046_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ─── Calibration structure ─────────────────────────────────────────────── */
typedef struct {
    int32_t  alphaX;   /* X scale numerator   */
    int32_t  betaX;    /* X offset            */
    int32_t  alphaY;   /* Y scale numerator   */
    int32_t  betaY;    /* Y offset            */
    int32_t  divider;  /* common denominator  */
    bool     valid;
} XPT2046_Calib_t;

/* ─── Touch point ───────────────────────────────────────────────────────── */
typedef struct {
    uint16_t x;        /* pixel x (0 … ILI9488_WIDTH-1)  */
    uint16_t y;        /* pixel y (0 … ILI9488_HEIGHT-1) */
    uint16_t z;        /* pressure (raw, 0 = not touched)  */
} XPT2046_Point_t;

/* ─── Public API ────────────────────────────────────────────────────────── */
void XPT2046_Init(SPI_HandleTypeDef *hspi);

bool XPT2046_IsTouched(void);

/**
 * @brief  Read raw ADC values (0–4095).
 * @retval true if a valid touch was detected.
 */
bool XPT2046_GetTouchRaw(uint16_t *rawX, uint16_t *rawY, uint16_t *rawZ);

/**
 * @brief  Read calibrated pixel coordinates.
 *         Returns false if not touched or calibration invalid.
 */
bool XPT2046_GetTouchPixel(XPT2046_Point_t *pt);

/**
 * @brief  Simple 3-point calibration routine.
 *         Caller must display cross-hair markers and call this with the three
 *         raw touch measurements collected in sequence.
 */
void XPT2046_SetCalibration(const XPT2046_Calib_t *cal);
void XPT2046_GetCalibration(XPT2046_Calib_t *cal);

/**
 * @brief  TOUCH_INT EXTI callback – call from HAL_GPIO_EXTI_Callback().
 */
void XPT2046_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif /* XPT2046_H */
