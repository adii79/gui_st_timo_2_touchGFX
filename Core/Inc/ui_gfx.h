/**
 ******************************************************************************
 * @file    ui_gfx.h
 * @brief   Global graphics helpers: bitmap renderer + slider widget
 *
 * Bitmap API
 * ----------
 *   UI_DrawBitmap(x, y, bmp, scale, fg)
 *     Place any UI_Bitmap_t anywhere on screen in any colour.
 *     Transparent mode: '0' bits are NOT painted (background shows through).
 *     Fast path: scale=1 uses DrawPixel per set bit only.
 *     Block path: scale>1 uses FillRect for each bit.
 *
 *   UI_DrawBitmapOpaque(x, y, bmp, scale, fg, bg)
 *     Same but '0' bits are painted in bg colour (solid rectangle result).
 *
 * Slider API
 * ----------
 *   UI_Slider_t  — stateful slider, stack-allocate one per slider widget
 *   UI_Slider_Init(s, x, y, w, min, max, initial)
 *   UI_Slider_Draw(s)          — full redraw (call once on init)
 *   UI_Slider_Touch(s, tx, ty) — feed touch pixel; returns 1 if value changed
 *   UI_Slider_Release(s)       — call on touch-up
 *   UI_Slider_GetValue(s)      — current int value
 *   UI_Slider_GetKnobX(s)      — current knob x pixel (for layout)
 *   UI_Slider_GetKnobY(s)      — knob y pixel (centre of track)
 ******************************************************************************
 */

#ifndef UI_GFX_H
#define UI_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "ui_bitmaps.h"
#include <stdint.h>
#include <stdbool.h>

/* ═══════════════════════════════════════════════════════════════════════════
   Bitmap renderer
   ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Draw a 1-bit bitmap at (x,y). Zero bits are transparent.
 * @param  x, y   Top-left pixel on display.
 * @param  bmp    Pointer to descriptor (data, w, h, bpr).
 * @param  scale  Integer scale factor (1 = 1:1, 2 = 2×2 blocks …).
 * @param  fg     RGB565 foreground colour for '1' bits.
 */
void UI_DrawBitmap(uint16_t x, uint16_t y,
                   const UI_Bitmap_t *bmp,
                   uint8_t scale,
                   uint16_t fg);

/**
 * @brief  Draw a 1-bit bitmap at (x,y). Zero bits are painted in bg.
 *         Use this when you want a solid bounding rectangle.
 */
void UI_DrawBitmapOpaque(uint16_t x, uint16_t y,
                         const UI_Bitmap_t *bmp,
                         uint8_t scale,
                         uint16_t fg, uint16_t bg);

/* ═══════════════════════════════════════════════════════════════════════════
   Slider widget
   ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    /* geometry */
    uint16_t track_x;      /* left edge of track                            */
    uint16_t track_y;      /* vertical centre of track                      */
    uint16_t track_w;      /* track length in pixels                        */

    /* value range */
    int16_t  val_min;
    int16_t  val_max;
    int16_t  value;        /* current value in [val_min .. val_max]         */

    /* internal: last drawn knob x (for dirty-rect) */
    uint16_t _knob_x;
    uint8_t  _dragging;    /* 1 while touch held inside slider              */
} UI_Slider_t;

/* Geometry constants (tweak to taste) */
#define UI_SLIDER_TRACK_H    6    /* track bar height pixels                */
#define UI_SLIDER_KNOB_R    12    /* knob circle radius                     */
#define UI_SLIDER_LABEL_H   14    /* height of value label above knob       */

/* Colours (RGB565) — override before including if needed */
#ifndef UI_SLD_COL_FILL
#define UI_SLD_COL_FILL    0x05FFu  /* cyan fill left of knob               */
#endif
#ifndef UI_SLD_COL_TRACK
#define UI_SLD_COL_TRACK   0x39E7u  /* grey unfilled right of knob          */
#endif
#ifndef UI_SLD_COL_KNOB
#define UI_SLD_COL_KNOB    0xFFFFu  /* white knob face                      */
#endif
#ifndef UI_SLD_COL_KNOB_BORDER
#define UI_SLD_COL_KNOB_BORDER 0x07FFu /* cyan ring                        */
#endif
#ifndef UI_SLD_COL_KNOB_DOT
#define UI_SLD_COL_KNOB_DOT 0x05FFu   /* inner accent dot                  */
#endif
#ifndef UI_SLD_COL_LABEL_FG
#define UI_SLD_COL_LABEL_FG 0xFFFFu
#endif
#ifndef UI_SLD_COL_LABEL_BG
#define UI_SLD_COL_LABEL_BG 0x0000u
#endif

/**
 * @brief  Initialise slider state. Call before UI_Slider_Draw().
 * @param  s         Pointer to caller-allocated UI_Slider_t.
 * @param  track_x   Left edge of track (pixel).
 * @param  track_y   Vertical centre of track (pixel).
 * @param  track_w   Track length in pixels.
 * @param  val_min   Minimum value (inclusive).
 * @param  val_max   Maximum value (inclusive).
 * @param  initial   Starting value (clamped to [min, max]).
 */
void UI_Slider_Init(UI_Slider_t *s,
                    uint16_t track_x, uint16_t track_y, uint16_t track_w,
                    int16_t val_min, int16_t val_max, int16_t initial);

/** @brief  Full redraw. Call once after UI_Slider_Init(). */
void UI_Slider_Draw(UI_Slider_t *s);

/**
 * @brief  Feed a touch pixel coordinate. Call every poll while touched.
 * @return 1 if value changed (dirty), 0 otherwise.
 */
uint8_t UI_Slider_Touch(UI_Slider_t *s, uint16_t tx, uint16_t ty);

/** @brief  Notify slider that finger was lifted. */
void UI_Slider_Release(UI_Slider_t *s);

/** @brief  Query current value. */
static inline int16_t  UI_Slider_GetValue(const UI_Slider_t *s) { return s->value;   }

/** @brief  Query knob centre-x pixel (useful for placing a label above it). */
static inline uint16_t UI_Slider_GetKnobX(const UI_Slider_t *s) { return s->_knob_x; }

/** @brief  Query knob centre-y pixel. */
static inline uint16_t UI_Slider_GetKnobY(const UI_Slider_t *s) { return s->track_y; }

#ifdef __cplusplus
}
#endif
#endif /* UI_GFX_H */
