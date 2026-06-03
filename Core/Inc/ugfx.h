///* ugfx.h — µGFX declarative widget engine */
//#ifndef UGFX_H
//#define UGFX_H
//
//#include <stdint.h>
//#include <stdbool.h>
//#include <string.h>
//#include "ili9488.h"
//#include "xpt2046.h"
//#include "ui_gfx.h"
//
///* ══════════════════════════════════════════════════════════════════════════
//   POOL SIZES
//   ══════════════════════════════════════════════════════════════════════════ */
//#define UGFX_MAX_SLIDERS   8
//#define UGFX_MAX_BUTTONS   16
//#define UGFX_MAX_ICONS     8
//#define UGFX_MAX_LABELS    8
//
///* ══════════════════════════════════════════════════════════════════════════
//   STYLE CONSTANTS
//   ══════════════════════════════════════════════════════════════════════════ */
//#define UGFX_KNOB_R        12u
//#define UGFX_TRACK_H        4u
//
//#define UGFX_COL_BG         0x0000u
//#define UGFX_COL_TRACK      0x39E7u
//#define UGFX_COL_FILL       0x07FFu
//#define UGFX_COL_KNOB       0xFFFFu
//#define UGFX_COL_LABEL      0xFFFFu
//#define UGFX_COL_BTN_IDLE   0x2104u
//#define UGFX_COL_BTN_PRESS  0x4208u
//#define UGFX_COL_BTN_BORDER 0x8410u
//#define UGFX_COL_BTN_TXT    0xFFFFu
//
///* ══════════════════════════════════════════════════════════════════════════
//   ENUMS
//   ══════════════════════════════════════════════════════════════════════════ */
//typedef enum { UGFX_HORIZONTAL = 0, UGFX_VERTICAL } ugfx_dir_t;
//
//typedef enum {
//    UGFX_ROT_0   = 0,
//    UGFX_ROT_90,
//    UGFX_ROT_180,
//    UGFX_ROT_270
//} ugfx_rot_t;
//
//typedef enum {
//    UGFX_TOUCH_NONE = 0x00,
//    UGFX_TOUCH_TAP  = 0x01,
//    UGFX_TOUCH_DRAG = 0x02,
//} ugfx_touch_mask_t;
//
///* ══════════════════════════════════════════════════════════════════════════
//   CALLBACKS
//   ══════════════════════════════════════════════════════════════════════════ */
//typedef void (*ugfx_value_cb)(int32_t value);
//typedef void (*ugfx_bg_redraw_fn)(void);
//
///* ══════════════════════════════════════════════════════════════════════════
//   WIDGET STRUCTS
//   ══════════════════════════════════════════════════════════════════════════ */
//
///* ── Slider ─────────────────────────────────────────────────────────────── */
//typedef struct ugfx_slider {
//    uint16_t          x, y, w, h;
//    ugfx_dir_t        dir;
//    int32_t           val_min, val_max, value;
//    uint16_t          col_track, col_fill, col_knob;
//    ugfx_touch_mask_t touch;
//    ugfx_value_cb     on_changed;
//    ugfx_bg_redraw_fn bg_redraw_fn;
//    const UI_Bitmap_t *knob_bmp;
//    uint8_t            knob_bmp_scale;
//    uint16_t           knob_bmp_col;
//    bool _active;
//    bool _dragging;
//} ugfx_slider_t;
//
///* ── Button ─────────────────────────────────────────────────────────────── */
//struct ugfx_button;
//typedef void (*ugfx_action_cb)(struct ugfx_button *btn);
//
//typedef struct ugfx_button {
//    uint16_t          x, y, w, h;
//    const char       *label;
//    uint8_t           label_size;
//    uint16_t          col_idle, col_press, col_border, col_text;
//    ugfx_touch_mask_t touch;
//    ugfx_action_cb    on_tap;
//    bool _active;
//    bool _pressed;
//} ugfx_button_t;
//
///* ── Icon ───────────────────────────────────────────────────────────────── */
//typedef struct ugfx_icon {
//    uint16_t           x, y;
//    const UI_Bitmap_t *bmp;
//    uint8_t            scale;
//    uint16_t           color;
//    bool               _active;
//} ugfx_icon_t;
//
///* ── Label ──────────────────────────────────────────────────────────────── */
//typedef struct ugfx_label {
//    uint16_t x, y;
//    char     text[32];
//    uint8_t  size;
//    uint16_t col_fg, col_bg;
//    bool     _active;
//    bool     _dirty;
//} ugfx_label_t;
//
///* ══════════════════════════════════════════════════════════════════════════
//   BUILDERS
//   ══════════════════════════════════════════════════════════════════════════ */
//
//typedef struct ugfx_slider_builder {
//    ugfx_slider_t cfg;
//    struct ugfx_slider_builder *(*frame)      (struct ugfx_slider_builder *, uint16_t w, uint16_t h);
//    struct ugfx_slider_builder *(*origin)     (struct ugfx_slider_builder *, uint16_t x, uint16_t y);
//    struct ugfx_slider_builder *(*direction)  (struct ugfx_slider_builder *, ugfx_dir_t);
//    struct ugfx_slider_builder *(*colors)     (struct ugfx_slider_builder *, uint16_t track, uint16_t fill, uint16_t knob);
//    struct ugfx_slider_builder *(*touchMask)  (struct ugfx_slider_builder *, ugfx_touch_mask_t);
//    struct ugfx_slider_builder *(*onChanged)  (struct ugfx_slider_builder *, ugfx_value_cb);
//    struct ugfx_slider_builder *(*bgRedraw)   (struct ugfx_slider_builder *, ugfx_bg_redraw_fn);
//    struct ugfx_slider_builder *(*knobBitmap) (struct ugfx_slider_builder *, const UI_Bitmap_t *, uint8_t scale, uint16_t col);
//    ugfx_slider_t              *(*build)      (struct ugfx_slider_builder *);
//} ugfx_slider_builder_t;
//
//typedef struct ugfx_button_builder {
//    ugfx_button_t cfg;
//    struct ugfx_button_builder *(*frame)     (struct ugfx_button_builder *, uint16_t w, uint16_t h);
//    struct ugfx_button_builder *(*origin)    (struct ugfx_button_builder *, uint16_t x, uint16_t y);
//    struct ugfx_button_builder *(*labelSize) (struct ugfx_button_builder *, uint8_t);
//    struct ugfx_button_builder *(*colors)    (struct ugfx_button_builder *, uint16_t idle, uint16_t press, uint16_t border, uint16_t text);
//    struct ugfx_button_builder *(*touchMask) (struct ugfx_button_builder *, ugfx_touch_mask_t);
//    struct ugfx_button_builder *(*onTap)     (struct ugfx_button_builder *, ugfx_action_cb);
//    ugfx_button_t              *(*build)     (struct ugfx_button_builder *);
//} ugfx_button_builder_t;
//
//typedef struct ugfx_icon_builder {
//    ugfx_icon_t cfg;
//    struct ugfx_icon_builder *(*origin) (struct ugfx_icon_builder *, uint16_t x, uint16_t y);
//    struct ugfx_icon_builder *(*scale)  (struct ugfx_icon_builder *, uint8_t);
//    struct ugfx_icon_builder *(*color)  (struct ugfx_icon_builder *, uint16_t);
//    ugfx_icon_t              *(*build)  (struct ugfx_icon_builder *);
//} ugfx_icon_builder_t;
//
//typedef struct ugfx_label_builder {
//    ugfx_label_t cfg;
//    struct ugfx_label_builder *(*origin) (struct ugfx_label_builder *, uint16_t x, uint16_t y);
//    struct ugfx_label_builder *(*color)  (struct ugfx_label_builder *, uint16_t fg, uint16_t bg);
//    struct ugfx_label_builder *(*size)   (struct ugfx_label_builder *, uint8_t);
//    ugfx_label_t              *(*build)  (struct ugfx_label_builder *);
//} ugfx_label_builder_t;
//
///* ══════════════════════════════════════════════════════════════════════════
//   PUBLIC API
//   ══════════════════════════════════════════════════════════════════════════ */
//void    UGFX_Init          (void);
//void    UGFX_Begin         (void);
//void    UGFX_Commit        (void);
//void    UGFX_Poll          (void);
//void    UGFX_SetRotation   (ugfx_rot_t rot);
//
//void    UGFX_SliderDraw    (ugfx_slider_t *s);
//void    UGFX_SliderSetValue(ugfx_slider_t *s, int32_t value);
//int32_t UGFX_SliderGetValue(const ugfx_slider_t *s);
//
//void    UGFX_ButtonDraw    (ugfx_button_t *b, bool pressed);
//void    UGFX_IconDraw      (ugfx_icon_t *ic);
//void    UGFX_LabelDraw     (ugfx_label_t *lbl);
//void    UGFX_LabelSetText  (ugfx_label_t *lbl, const char *text);
//
//ugfx_slider_builder_t *Slider(int32_t min, int32_t max, int32_t initial);
//ugfx_button_builder_t *Button(const char *label);
//ugfx_icon_builder_t   *Icon  (const UI_Bitmap_t *bmp);
//ugfx_label_builder_t  *Label (const char *text);
//
//#endif /* UGFX_H */





/* ugfx.h — µGFX declarative widget engine */
#ifndef UGFX_H
#define UGFX_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ili9488.h"
#include "xpt2046.h"
#include "ui_gfx.h"

/* ══════════════════════════════════════════════════════════════════════════
   POOL SIZES
   ══════════════════════════════════════════════════════════════════════════ */
#define UGFX_MAX_SLIDERS   8
#define UGFX_MAX_BUTTONS   16
#define UGFX_MAX_ICONS     8
#define UGFX_MAX_LABELS    8

/* ══════════════════════════════════════════════════════════════════════════
   STYLE CONSTANTS
   ══════════════════════════════════════════════════════════════════════════ */
#define UGFX_KNOB_R        12u
#define UGFX_TRACK_H        4u

#define UGFX_COL_BG          0x0000u
#define UGFX_COL_TRACK       0x39E7u
#define UGFX_COL_FILL        0x07FFu
#define UGFX_COL_KNOB        0xFFFFu
#define UGFX_COL_LABEL       0xFFFFu
#define UGFX_COL_BTN_IDLE    0x2104u
#define UGFX_COL_BTN_PRESS   0x4208u
#define UGFX_COL_BTN_BORDER  0x8410u
#define UGFX_COL_BTN_TXT     0xFFFFu

/* Pass as col_track or col_fill to skip drawing that layer entirely.
   The gradient pill drawn by bg_redraw_fn shows through underneath. */
#define UGFX_COL_TRANSPARENT 0xBEEFu

/* ══════════════════════════════════════════════════════════════════════════
   ENUMS
   ══════════════════════════════════════════════════════════════════════════ */
typedef enum { UGFX_HORIZONTAL = 0, UGFX_VERTICAL } ugfx_dir_t;

typedef enum {
    UGFX_ROT_0   = 0,
    UGFX_ROT_90,
    UGFX_ROT_180,
    UGFX_ROT_270
} ugfx_rot_t;

typedef enum {
    UGFX_TOUCH_NONE = 0x00,
    UGFX_TOUCH_TAP  = 0x01,
    UGFX_TOUCH_DRAG = 0x02,
} ugfx_touch_mask_t;

/* ══════════════════════════════════════════════════════════════════════════
   CALLBACKS
   ══════════════════════════════════════════════════════════════════════════ */
typedef void (*ugfx_value_cb)(int32_t value);
typedef void (*ugfx_bg_redraw_fn)(void);

/* ══════════════════════════════════════════════════════════════════════════
   WIDGET STRUCTS
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider ─────────────────────────────────────────────────────────────── */
typedef struct ugfx_slider {
    uint16_t          x, y, w, h;
    ugfx_dir_t        dir;
    int32_t           val_min, val_max, value;
    uint16_t          col_track, col_fill, col_knob;
    ugfx_touch_mask_t touch;
    ugfx_value_cb     on_changed;
    ugfx_bg_redraw_fn bg_redraw_fn;
    const UI_Bitmap_t *knob_bmp;
    uint8_t            knob_bmp_scale;
    uint16_t           knob_bmp_col;
    bool _active;
    bool _dragging;
} ugfx_slider_t;

/* ── Button ─────────────────────────────────────────────────────────────── */
struct ugfx_button;
typedef void (*ugfx_action_cb)(struct ugfx_button *btn);

typedef struct ugfx_button {
    uint16_t          x, y, w, h;
    const char       *label;
    uint8_t           label_size;
    uint16_t          col_idle, col_press, col_border, col_text;
    ugfx_touch_mask_t touch;
    ugfx_action_cb    on_tap;
    bool _active;
    bool _pressed;
} ugfx_button_t;

/* ── Icon ───────────────────────────────────────────────────────────────── */
typedef struct ugfx_icon {
    uint16_t           x, y;
    const UI_Bitmap_t *bmp;
    uint8_t            scale;
    uint16_t           color;
    bool               _active;
} ugfx_icon_t;

/* ── Label ──────────────────────────────────────────────────────────────── */
typedef struct ugfx_label {
    uint16_t x, y;
    char     text[32];
    uint8_t  size;
    uint16_t col_fg, col_bg;
    bool     _active;
    bool     _dirty;
} ugfx_label_t;

/* ══════════════════════════════════════════════════════════════════════════
   BUILDERS
   ══════════════════════════════════════════════════════════════════════════ */

typedef struct ugfx_slider_builder {
    ugfx_slider_t cfg;
    struct ugfx_slider_builder *(*frame)      (struct ugfx_slider_builder *, uint16_t w, uint16_t h);
    struct ugfx_slider_builder *(*origin)     (struct ugfx_slider_builder *, uint16_t x, uint16_t y);
    struct ugfx_slider_builder *(*direction)  (struct ugfx_slider_builder *, ugfx_dir_t);
    struct ugfx_slider_builder *(*colors)     (struct ugfx_slider_builder *, uint16_t track, uint16_t fill, uint16_t knob);
    struct ugfx_slider_builder *(*touchMask)  (struct ugfx_slider_builder *, ugfx_touch_mask_t);
    struct ugfx_slider_builder *(*onChanged)  (struct ugfx_slider_builder *, ugfx_value_cb);
    struct ugfx_slider_builder *(*bgRedraw)   (struct ugfx_slider_builder *, ugfx_bg_redraw_fn);
    struct ugfx_slider_builder *(*knobBitmap) (struct ugfx_slider_builder *, const UI_Bitmap_t *, uint8_t scale, uint16_t col);
    ugfx_slider_t              *(*build)      (struct ugfx_slider_builder *);
} ugfx_slider_builder_t;

typedef struct ugfx_button_builder {
    ugfx_button_t cfg;
    struct ugfx_button_builder *(*frame)     (struct ugfx_button_builder *, uint16_t w, uint16_t h);
    struct ugfx_button_builder *(*origin)    (struct ugfx_button_builder *, uint16_t x, uint16_t y);
    struct ugfx_button_builder *(*labelSize) (struct ugfx_button_builder *, uint8_t);
    struct ugfx_button_builder *(*colors)    (struct ugfx_button_builder *, uint16_t idle, uint16_t press, uint16_t border, uint16_t text);
    struct ugfx_button_builder *(*touchMask) (struct ugfx_button_builder *, ugfx_touch_mask_t);
    struct ugfx_button_builder *(*onTap)     (struct ugfx_button_builder *, ugfx_action_cb);
    ugfx_button_t              *(*build)     (struct ugfx_button_builder *);
} ugfx_button_builder_t;

typedef struct ugfx_icon_builder {
    ugfx_icon_t cfg;
    struct ugfx_icon_builder *(*origin) (struct ugfx_icon_builder *, uint16_t x, uint16_t y);
    struct ugfx_icon_builder *(*scale)  (struct ugfx_icon_builder *, uint8_t);
    struct ugfx_icon_builder *(*color)  (struct ugfx_icon_builder *, uint16_t);
    ugfx_icon_t              *(*build)  (struct ugfx_icon_builder *);
} ugfx_icon_builder_t;

typedef struct ugfx_label_builder {
    ugfx_label_t cfg;
    struct ugfx_label_builder *(*origin) (struct ugfx_label_builder *, uint16_t x, uint16_t y);
    struct ugfx_label_builder *(*color)  (struct ugfx_label_builder *, uint16_t fg, uint16_t bg);
    struct ugfx_label_builder *(*size)   (struct ugfx_label_builder *, uint8_t);
    ugfx_label_t              *(*build)  (struct ugfx_label_builder *);
} ugfx_label_builder_t;

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════════════════════════ */
void    UGFX_Init          (void);
void    UGFX_Begin         (void);
void    UGFX_Commit        (void);
void    UGFX_Poll          (void);
void    UGFX_SetRotation   (ugfx_rot_t rot);

void    UGFX_SliderDraw    (ugfx_slider_t *s);
void    UGFX_SliderSetValue(ugfx_slider_t *s, int32_t value);
int32_t UGFX_SliderGetValue(const ugfx_slider_t *s);

void    UGFX_ButtonDraw    (ugfx_button_t *b, bool pressed);
void    UGFX_IconDraw      (ugfx_icon_t *ic);
void    UGFX_LabelDraw     (ugfx_label_t *lbl);
void    UGFX_LabelSetText  (ugfx_label_t *lbl, const char *text);

ugfx_slider_builder_t *Slider(int32_t min, int32_t max, int32_t initial);
ugfx_button_builder_t *Button(const char *label);
ugfx_icon_builder_t   *Icon  (const UI_Bitmap_t *bmp);
ugfx_label_builder_t  *Label (const char *text);

#endif /* UGFX_H */
