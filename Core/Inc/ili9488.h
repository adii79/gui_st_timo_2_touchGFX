/**
 ******************************************************************************
 * @file    ili9488.h
 * @brief   ILI9488 3.5" SPI TFT Display Driver (480x320, 18-bit colour)
 *          Hardware: KMRTM35018-SPI v2.0  |  MCU: STM32F429ZGTx
 *
 * Pin Mapping (from CubeMX / main.h):
 *   SPI1  SCK  -> PB3  (DISPL_SCK)
 *   SPI1  MISO -> PB4  (TOUCH_MISO – shared bus)
 *   SPI1  MOSI -> PB5  (DISPL_MOSI)
 *   DISPL_CS   -> PF1  (active LOW)
 *   DISPL_DC   -> PC15 (LOW = command, HIGH = data)
 *   DISPL_RST  -> PF0  (active LOW)
 *   DISPL_LED  -> PC14 (HIGH = backlight on)
 *
 * TouchGFX compatibility:
 *   - Implements ILI9488_CopyFrameBufferBlockToLCD() for HAL integration
 *   - 18-bit RGB666 pixel format (3 bytes per pixel over SPI)
 *   - Colour helpers convert RGB565 → RGB666 for TouchGFX framebuffer support
 ******************************************************************************
 */

#ifndef ILI9488_H
#define ILI9488_H
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Includes ──────────────────────────────────────────────────────────── */
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ─── Display geometry ──────────────────────────────────────────────────── */
//#define ILI9488_WIDTH   320u
//#define ILI9488_HEIGHT  480u

#define ILI9488_WIDTH   480u
#define ILI9488_HEIGHT  320u

/* ─── Colour macros (RGB565 input → 32-bit packed storage) ─────────────── */
#define ILI9488_COLOR_BLACK   0x0000u
#define ILI9488_COLOR_WHITE   0xFFFFu
#define ILI9488_COLOR_RED     0xF800u
#define ILI9488_COLOR_GREEN   0x07E0u
#define ILI9488_COLOR_BLUE    0x001Fu
#define ILI9488_COLOR_YELLOW  0xFFE0u
#define ILI9488_COLOR_CYAN    0x07FFu
#define ILI9488_COLOR_MAGENTA 0xF81Fu

/* Expand RGB565 to RGB666 (3 separate bytes, MSB-aligned) */
#define RGB565_TO_R6(c)  (((c) & 0xF800u) >> 8u)
#define RGB565_TO_G6(c)  (((c) & 0x07E0u) >> 3u)
#define RGB565_TO_B6(c)  (((c) & 0x001Fu) << 3u)

/* ─── Orientation ───────────────────────────────────────────────────────── */
typedef enum {
    ILI9488_ORIENT_LANDSCAPE = 0,   /* 480 wide, 320 tall  (default) */
    ILI9488_ORIENT_PORTRAIT,        /* 320 wide, 480 tall             */
    ILI9488_ORIENT_LANDSCAPE_FLIP,
    ILI9488_ORIENT_PORTRAIT_FLIP
} ILI9488_Orientation_t;

/* ─── Public API ────────────────────────────────────────────────────────── */

/* Initialisation */
void ILI9488_Init(SPI_HandleTypeDef *hspi);
void ILI9488_SetOrientation(ILI9488_Orientation_t orient);
void ILI9488_SetBacklight(bool on);

/* Drawing primitives */
void ILI9488_FillScreen(uint16_t colour);
void ILI9488_DrawPixel(uint16_t x, uint16_t y, uint16_t colour);
void ILI9488_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
void ILI9488_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
void ILI9488_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t colour);
void ILI9488_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t colour);
void ILI9488_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t colour);
void ILI9488_DrawCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t colour);
void ILI9488_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t colour);

/* Text rendering (built-in 5×7 font) */
void ILI9488_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t size);
void ILI9488_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t size);

/* Raw pixel burst – used by TouchGFX HAL */
void ILI9488_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9488_WritePixels_RGB565(const uint16_t *buf, uint32_t count);
void ILI9488_WritePixels_RGB666(const uint8_t *buf, uint32_t byteCount);

/* TouchGFX HAL integration hook */
void ILI9488_CopyFrameBufferBlockToLCD(const uint16_t *pSrc,
                                        uint16_t x, uint16_t y,
                                        uint16_t width, uint16_t height);

/* Low-level helpers (exposed for XPT2046 bus sharing) */
void ILI9488_CS_Assert(void);
void ILI9488_CS_Deassert(void);

//#ifndef ILI9488_H
//#define ILI9488_H
//
//#include <stdint.h>
//
//void ILI9488_FlushDMA(lv_disp_drv_t *drv,
//                      const lv_area_t *area,
//                      lv_color_t *color_p);

//#endif

#ifdef __cplusplus
}
#endif
#endif /* ILI9488_H */
