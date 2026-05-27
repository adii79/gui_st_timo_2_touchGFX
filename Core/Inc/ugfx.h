/**
 ******************************************************************************
 * @file    ugfx.h
 * @brief   µGFX — Lightweight Declarative UI Library for STM32 + ILI9488
 *
 * SwiftUI-inspired builder API over ILI9488 + XPT2046.
 * No RTOS. No heap. All widgets in static pools.
 *
 * Quick-start
 * ──────────────────────────────────────────────────────────────────────────
 *
 *   // 1. Init once after ILI9488_Init() and XPT2046_Init():
 *   UGFX_Init();
 *
 *   // 2. Describe your screen (call once, or again to rebuild):
 *   UGFX_Begin();
 *
 *     ugfx_slider_t *vol =
 *         Slider(0, 100, 60)                  // min, max, initial value
 *           .frame(260, 18)                   // width, height
 *           .origin(40, 80)                   // x, y on screen
 *           .direction(UGFX_HORIZONTAL)
 *           .onChanged(my_vol_cb)             // optional callback
 *           .build();
 *
 *     ugfx_button_t *btn =
 *         Button("PRESS ME")
 *           .frame(150, 46)
 *           .origin(30, 160)
 *           .onTap(my_btn_cb)
 *           .build();
 *
 *     ugfx_icon_t *ico =
 *         Icon(&bmp_home)
 *           .origin(10, 10)
 *           .scale(2)
 *           .color(COL_CYAN)
 *           .build();
 *
 *     ugfx_label_t *lbl =
 *         Label("Hello")
 *           .origin(10, 200)
 *           .color(COL_WHITE)
 *           .size(2)
 *           .build();
 *
 *   UGFX_Commit();  // renders everything once
 *
 *   // 3. In the main loop:
 *   while (1) { UGFX_Poll(); }
 *
 * Touch masks
 * ──────────────────────────────────────────────────────────────────────────
 *   Each widget has a touch mask:  UGFX_TOUCH_NONE | UGFX_TOUCH_TAP |
 *                                   UGFX_TOUCH_DRAG | UGFX_TOUCH_BOTH
 *   Sliders default to DRAG, buttons default to TAP.
 *   Override with .touchMask(UGFX_TOUCH_BOTH).
 *
 * Rotation
 * ──────────────────────────────────────────────────────────────────────────
 *   UGFX_SetRotation(UGFX_ROT_0 | UGFX_ROT_90 | UGFX_ROT_180 | UGFX_ROT_270)
 *   Applies a coordinate transform to every widget origin + touch point.
 *   Sliders re-orient their drag axis automatically.
 *
 ******************************************************************************
 */
#ifndef UGFX_H
#define UGFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "ili9488.h"
#include "xpt2046.h"
#include "ui_gfx.h"   /* UI_Bitmap_t, UI_DrawBitmap */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ══════════════════════════════════════════════════════════════════════════
   CONFIGURATION  — edit these to suit your project
   ══════════════════════════════════════════════════════════════════════════ */

#define UGFX_MAX_SLIDERS    16u
#define UGFX_MAX_BUTTONS    16u
#define UGFX_MAX_ICONS      16u
#define UGFX_MAX_LABELS     16u

/* Default theme colours (RGB565) */
#define UGFX_COL_BG         0x0000u   /* screen background                  */
#define UGFX_COL_TRACK      0x2104u   /* slider track                       */
#define UGFX_COL_KNOB       0x07FFu   /* slider knob (cyan)                 */
#define UGFX_COL_FILL       0x0379u   /* slider fill (dark cyan)            */
#define UGFX_COL_BTN_IDLE   0x2104u   /* button face idle                   */
#define UGFX_COL_BTN_PRESS  0x4228u   /* button face pressed                */
#define UGFX_COL_BTN_BORDER 0xFFFFu   /* button border idle                 */
#define UGFX_COL_BTN_TXT    0xFFFFu   /* button label                       */
#define UGFX_COL_LABEL      0xFFFFu   /* default label colour               */

/* Slider geometry */
#define UGFX_KNOB_R         9u        /* knob radius (pixels)               */
#define UGFX_TRACK_H        4u        /* track thickness                    */

/* ══════════════════════════════════════════════════════════════════════════
   ENUMS
   ══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    UGFX_HORIZONTAL = 0,
    UGFX_VERTICAL
} ugfx_dir_t;

typedef enum {
    UGFX_ROT_0   = 0,
    UGFX_ROT_90,
    UGFX_ROT_180,
    UGFX_ROT_270
} ugfx_rot_t;

typedef enum {
    UGFX_TOUCH_NONE = 0x00u,
    UGFX_TOUCH_TAP  = 0x01u,
    UGFX_TOUCH_DRAG = 0x02u,
    UGFX_TOUCH_BOTH = 0x03u
} ugfx_touch_mask_t;

/* ══════════════════════════════════════════════════════════════════════════
   WIDGET STRUCTS  (treat as opaque — use the builder API)
   ══════════════════════════════════════════════════════════════════════════ */

typedef void (*ugfx_value_cb)(int32_t value);   /* slider changed          */
typedef void (*ugfx_action_cb)(void);            /* button tapped           */

/* ── Slider ────────────────────────────────────────────────────────────── */
typedef struct {
    /* geometry */
    uint16_t x, y;          /* top-left origin                             */
    uint16_t w, h;          /* bounding frame (knob clips inside)          */
    ugfx_dir_t dir;         /* HORIZONTAL or VERTICAL                      */

    /* value */
    int32_t val_min, val_max, value;

    /* style */
    uint16_t col_track, col_fill, col_knob;

    /* interaction */
    ugfx_touch_mask_t touch;
    ugfx_value_cb on_changed;

    /* internal state */
    bool _dragging;
    bool _active;           /* registered and alive                        */
} ugfx_slider_t;

/* ── Button ────────────────────────────────────────────────────────────── */
typedef struct {
    uint16_t x, y, w, h;
    const char *label;
    uint8_t label_size;

    uint16_t col_idle, col_press, col_border, col_text;

    ugfx_touch_mask_t touch;
    ugfx_action_cb on_tap;

    bool _pressed;
    bool _active;
} ugfx_button_t;

/* ── Icon (1-bit bitmap) ───────────────────────────────────────────────── */
typedef struct {
    uint16_t x, y;
    const UI_Bitmap_t *bmp;
    uint8_t  scale;
    uint16_t color;
    bool _active;
} ugfx_icon_t;

/* ── Label ─────────────────────────────────────────────────────────────── */
typedef struct {
    uint16_t x, y;
    char text[64];
    uint16_t col_fg, col_bg;
    uint8_t  size;
    bool _active;
    bool _dirty;
} ugfx_label_t;

/* ══════════════════════════════════════════════════════════════════════════
   BUILDER STRUCTS  (returned by Slider() / Button() / Icon() / Label())
   Each field is the "pending" config; call .build() to commit.
   ══════════════════════════════════════════════════════════════════════════ */

typedef struct _slider_builder ugfx_slider_builder_t;
struct _slider_builder {
    ugfx_slider_t cfg;
    ugfx_slider_builder_t *(*frame)      (ugfx_slider_builder_t*, uint16_t w, uint16_t h);
    ugfx_slider_builder_t *(*origin)     (ugfx_slider_builder_t*, uint16_t x, uint16_t y);
    ugfx_slider_builder_t *(*direction)  (ugfx_slider_builder_t*, ugfx_dir_t d);
    ugfx_slider_builder_t *(*colors)     (ugfx_slider_builder_t*, uint16_t track, uint16_t fill, uint16_t knob);
    ugfx_slider_builder_t *(*touchMask)  (ugfx_slider_builder_t*, ugfx_touch_mask_t m);
    ugfx_slider_builder_t *(*onChanged)  (ugfx_slider_builder_t*, ugfx_value_cb cb);
    ugfx_slider_t         *(*build)      (ugfx_slider_builder_t*);
};

typedef struct _button_builder ugfx_button_builder_t;
struct _button_builder {
    ugfx_button_t cfg;
    ugfx_button_builder_t *(*frame)      (ugfx_button_builder_t*, uint16_t w, uint16_t h);
    ugfx_button_builder_t *(*origin)     (ugfx_button_builder_t*, uint16_t x, uint16_t y);
    ugfx_button_builder_t *(*labelSize)  (ugfx_button_builder_t*, uint8_t s);
    ugfx_button_builder_t *(*colors)     (ugfx_button_builder_t*, uint16_t idle, uint16_t press, uint16_t border, uint16_t text);
    ugfx_button_builder_t *(*touchMask)  (ugfx_button_builder_t*, ugfx_touch_mask_t m);
    ugfx_button_builder_t *(*onTap)      (ugfx_button_builder_t*, ugfx_action_cb cb);
    ugfx_button_t         *(*build)      (ugfx_button_builder_t*);
};

typedef struct _icon_builder ugfx_icon_builder_t;
struct _icon_builder {
    ugfx_icon_t cfg;
    ugfx_icon_builder_t *(*origin)  (ugfx_icon_builder_t*, uint16_t x, uint16_t y);
    ugfx_icon_builder_t *(*scale)   (ugfx_icon_builder_t*, uint8_t s);
    ugfx_icon_builder_t *(*color)   (ugfx_icon_builder_t*, uint16_t c);
    ugfx_icon_t         *(*build)   (ugfx_icon_builder_t*);
};

typedef struct _label_builder ugfx_label_builder_t;
struct _label_builder {
    ugfx_label_t cfg;
    ugfx_label_builder_t *(*origin)  (ugfx_label_builder_t*, uint16_t x, uint16_t y);
    ugfx_label_builder_t *(*color)   (ugfx_label_builder_t*, uint16_t fg, uint16_t bg);
    ugfx_label_builder_t *(*size)    (ugfx_label_builder_t*, uint8_t s);
    ugfx_label_t         *(*build)   (ugfx_label_builder_t*);
};

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════════════════════════ */

/* Life-cycle */
void UGFX_Init(void);
void UGFX_Begin(void);       /* clear all pending widget registrations     */
void UGFX_Commit(void);      /* draw all registered widgets                */
void UGFX_Poll(void);        /* call every loop iteration                  */

/* Rotation */
void UGFX_SetRotation(ugfx_rot_t rot);

/* Builder entry-points — these return a builder you chain and then .build() */
ugfx_slider_builder_t *Slider(int32_t min, int32_t max, int32_t initial);
ugfx_button_builder_t *Button(const char *label);
ugfx_icon_builder_t   *Icon  (const UI_Bitmap_t *bmp);
ugfx_label_builder_t  *Label (const char *text);

/* Runtime helpers — update a live widget without full redraw */
void UGFX_SliderSetValue(ugfx_slider_t *s, int32_t value);
int32_t UGFX_SliderGetValue(const ugfx_slider_t *s);

void UGFX_LabelSetText(ugfx_label_t *lbl, const char *text);

/* Manual partial redraw (dirty-rect) */
void UGFX_SliderDraw(ugfx_slider_t *s);
void UGFX_ButtonDraw(ugfx_button_t *b, bool pressed);
void UGFX_IconDraw  (ugfx_icon_t *ic);
void UGFX_LabelDraw (ugfx_label_t *lbl);

#ifdef __cplusplus
}
#endif
#endif /* UGFX_H */