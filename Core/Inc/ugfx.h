/**
 ******************************************************************************
 * @file    ugfx.h
 * @brief   µGFX — declarative widget engine header
 *          Supports: Slider, Button, Icon, Label
 *          All callbacks receive a pointer to the widget that fired them.
 ******************************************************************************
 */

#ifndef UGFX_H
#define UGFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ili9488.h"
#include "xpt2046.h"
#include "ui_gfx.h"   /* UI_DrawBitmap, UI_Bitmap_t */

/* ══════════════════════════════════════════════════════════════════════════
   POOL SIZES  — increase if you need more widgets
   ══════════════════════════════════════════════════════════════════════════ */
#define UGFX_MAX_SLIDERS   8u
#define UGFX_MAX_BUTTONS   8u
#define UGFX_MAX_ICONS     8u
#define UGFX_MAX_LABELS    8u

/* ══════════════════════════════════════════════════════════════════════════
   STYLE CONSTANTS  — edit to restyle everything at once
   ══════════════════════════════════════════════════════════════════════════ */
#define UGFX_KNOB_R        12u          /* slider knob radius, pixels       */
#define UGFX_TRACK_H        4u          /* slider track thickness, pixels   */

/* Colour palette (RGB565) */
#define UGFX_COL_BG         0x0000u     /* background fill (erase)          */
#define UGFX_COL_TRACK      0x4208u     /* slider track (dark grey)         */
#define UGFX_COL_FILL       0x051Fu     /* slider filled region (blue)      */
#define UGFX_COL_KNOB       0xFFFFu     /* slider knob (white)              */
#define UGFX_COL_LABEL      0xFFFFu     /* label text default               */

#define UGFX_COL_BTN_IDLE   0x2945u     /* button face — idle               */
#define UGFX_COL_BTN_PRESS  0x051Fu     /* button face — pressed            */
#define UGFX_COL_BTN_BORDER 0x7BEFu     /* button border                    */
#define UGFX_COL_BTN_TXT    0xFFFFu     /* button label text                */

/* ══════════════════════════════════════════════════════════════════════════
   ENUMERATIONS
   ══════════════════════════════════════════════════════════════════════════ */

/** Rotation applied to widget coordinate space */
typedef enum {
    UGFX_ROT_0   = 0,
    UGFX_ROT_90,
    UGFX_ROT_180,
    UGFX_ROT_270
} ugfx_rot_t;

/** Slider orientation */
typedef enum {
    UGFX_HORIZONTAL = 0,
    UGFX_VERTICAL
} ugfx_dir_t;

/** Which touch gestures a widget responds to */
typedef enum {
    UGFX_TOUCH_NONE = 0x00u,
    UGFX_TOUCH_TAP  = 0x01u,
    UGFX_TOUCH_DRAG = 0x02u
} ugfx_touch_mask_t;

/* ══════════════════════════════════════════════════════════════════════════
   FORWARD DECLARATIONS  (needed for callback typedefs below)
   ══════════════════════════════════════════════════════════════════════════ */
typedef struct ugfx_slider ugfx_slider_t;
typedef struct ugfx_button ugfx_button_t;

/* ══════════════════════════════════════════════════════════════════════════
   CALLBACK TYPES
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Slider value-changed callback.
 * @param  value   New slider value (already clamped to [val_min, val_max]).
 */
typedef void (*ugfx_value_cb)(int32_t value);

/**
 * @brief  Button tap callback.
 * @param  btn     Pointer to the button that was tapped.
 *                 The callback is free to mutate btn->label, btn->col_idle,
 *                 btn->col_press, etc. — the engine redraws after returning.
 */
typedef void (*ugfx_action_cb)(ugfx_button_t *btn);

/* ══════════════════════════════════════════════════════════════════════════
   WIDGET STRUCTS
   ══════════════════════════════════════════════════════════════════════════ */

/** Slider widget */
struct ugfx_slider {
    /* Geometry */
    uint16_t x, y;          /* top-left origin in logical (widget) coords  */
    uint16_t w, h;          /* bounding box                                */
    ugfx_dir_t dir;         /* UGFX_HORIZONTAL or UGFX_VERTICAL            */

    /* Value */
    int32_t val_min;
    int32_t val_max;
    int32_t value;

    /* Appearance */
    uint16_t col_track;
    uint16_t col_fill;
    uint16_t col_knob;

    /* Interaction */
    ugfx_touch_mask_t touch;
    ugfx_value_cb     on_changed;

    /* Internal state — do not write directly */
    bool _active;
    bool _dragging;
};

/** Button widget */
struct ugfx_button {
    /* Geometry */
    uint16_t x, y;
    uint16_t w, h;

    /* Appearance */
    const char *label;
    uint8_t     label_size;    /* font scale factor (1 = 6×8 px per char)  */
    uint16_t    col_idle;
    uint16_t    col_press;
    uint16_t    col_border;
    uint16_t    col_text;

    /* Interaction */
    ugfx_touch_mask_t touch;
    ugfx_action_cb    on_tap;  /* called on press, receives (ugfx_button_t*) */

    /* Internal state — do not write directly */
    bool _active;
    bool _pressed;
};

/** Icon widget (monochrome bitmap) */
typedef struct {
    uint16_t           x, y;
    const UI_Bitmap_t *bmp;
    uint8_t            scale;
    uint16_t           color;
    bool               _active;
} ugfx_icon_t;

/** Label widget */
typedef struct {
    uint16_t x, y;
    char     text[32];
    uint8_t  size;
    uint16_t col_fg;
    uint16_t col_bg;
    bool     _active;
    bool     _dirty;
} ugfx_label_t;

/* ══════════════════════════════════════════════════════════════════════════
   BUILDER STRUCTS  (fluent / method-chaining API)
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider builder ──────────────────────────────────────────────────────── */
typedef struct ugfx_slider_builder ugfx_slider_builder_t;
struct ugfx_slider_builder {
    ugfx_slider_t cfg;

    ugfx_slider_builder_t *(*frame)    (ugfx_slider_builder_t *, uint16_t w, uint16_t h);
    ugfx_slider_builder_t *(*origin)   (ugfx_slider_builder_t *, uint16_t x, uint16_t y);
    ugfx_slider_builder_t *(*direction)(ugfx_slider_builder_t *, ugfx_dir_t);
    ugfx_slider_builder_t *(*colors)   (ugfx_slider_builder_t *, uint16_t track,
                                        uint16_t fill, uint16_t knob);
    ugfx_slider_builder_t *(*touchMask)(ugfx_slider_builder_t *, ugfx_touch_mask_t);
    ugfx_slider_builder_t *(*onChanged)(ugfx_slider_builder_t *, ugfx_value_cb);
    ugfx_slider_t         *(*build)    (ugfx_slider_builder_t *);
};

/* ── Button builder ──────────────────────────────────────────────────────── */
typedef struct ugfx_button_builder ugfx_button_builder_t;
struct ugfx_button_builder {
    ugfx_button_t cfg;

    ugfx_button_builder_t *(*frame)    (ugfx_button_builder_t *, uint16_t w, uint16_t h);
    ugfx_button_builder_t *(*origin)   (ugfx_button_builder_t *, uint16_t x, uint16_t y);
    ugfx_button_builder_t *(*labelSize)(ugfx_button_builder_t *, uint8_t s);
    ugfx_button_builder_t *(*colors)   (ugfx_button_builder_t *, uint16_t idle,
                                        uint16_t press, uint16_t border, uint16_t text);
    ugfx_button_builder_t *(*touchMask)(ugfx_button_builder_t *, ugfx_touch_mask_t);
    ugfx_button_builder_t *(*onTap)    (ugfx_button_builder_t *, ugfx_action_cb);
    ugfx_button_t         *(*build)    (ugfx_button_builder_t *);
};

/* ── Icon builder ────────────────────────────────────────────────────────── */
typedef struct ugfx_icon_builder ugfx_icon_builder_t;
struct ugfx_icon_builder {
    ugfx_icon_t cfg;

    ugfx_icon_builder_t *(*origin)(ugfx_icon_builder_t *, uint16_t x, uint16_t y);
    ugfx_icon_builder_t *(*scale) (ugfx_icon_builder_t *, uint8_t s);
    ugfx_icon_builder_t *(*color) (ugfx_icon_builder_t *, uint16_t c);
    ugfx_icon_t         *(*build) (ugfx_icon_builder_t *);
};

/* ── Label builder ───────────────────────────────────────────────────────── */
typedef struct ugfx_label_builder ugfx_label_builder_t;
struct ugfx_label_builder {
    ugfx_label_t cfg;

    ugfx_label_builder_t *(*origin)(ugfx_label_builder_t *, uint16_t x, uint16_t y);
    ugfx_label_builder_t *(*color) (ugfx_label_builder_t *, uint16_t fg, uint16_t bg);
    ugfx_label_builder_t *(*size)  (ugfx_label_builder_t *, uint8_t s);
    ugfx_label_t         *(*build) (ugfx_label_builder_t *);
};

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════════════════════════ */

/* Lifecycle */
void UGFX_Init       (void);
void UGFX_Begin      (void);
void UGFX_Commit     (void);
void UGFX_Poll       (void);
void UGFX_SetRotation(ugfx_rot_t rot);

/* Runtime widget control */
void    UGFX_SliderSetValue (ugfx_slider_t *s, int32_t value);
int32_t UGFX_SliderGetValue (const ugfx_slider_t *s);
void    UGFX_LabelSetText   (ugfx_label_t *lbl, const char *text);

/* Draw helpers (public so app code can force a redraw) */
void UGFX_SliderDraw(ugfx_slider_t *s);
void UGFX_ButtonDraw(ugfx_button_t *b, bool pressed);
void UGFX_IconDraw  (ugfx_icon_t   *ic);
void UGFX_LabelDraw (ugfx_label_t  *lbl);

/* Builder factory functions */
ugfx_slider_builder_t *Slider(int32_t min, int32_t max, int32_t initial);
ugfx_button_builder_t *Button(const char *label);
ugfx_icon_builder_t   *Icon  (const UI_Bitmap_t *bmp);
ugfx_label_builder_t  *Label (const char *text);

#ifdef __cplusplus
}
#endif

#endif /* UGFX_H */
