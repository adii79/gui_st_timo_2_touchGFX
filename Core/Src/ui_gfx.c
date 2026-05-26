/**
 ******************************************************************************
 * @file    ui_gfx.c
 * @brief   Bitmap renderer + slider widget implementation
 ******************************************************************************
 */

#include "ui_gfx.h"
#include "ili9488.h"
#include <stdio.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
   Bitmap renderer
   ═══════════════════════════════════════════════════════════════════════════ */

void UI_DrawBitmap(uint16_t x, uint16_t y,
                   const UI_Bitmap_t *bmp,
                   uint8_t scale,
                   uint16_t fg)
{
    if (!bmp || !bmp->data) return;

    for (uint8_t row = 0; row < bmp->h; row++) {
        for (uint8_t col = 0; col < bmp->w; col++) {
            /* MSB of each byte = leftmost pixel */
            uint8_t byte_idx = col / 8u;
            uint8_t bit_pos  = 7u - (col % 8u);
            uint8_t bit = (bmp->data[row * bmp->bpr + byte_idx] >> bit_pos) & 0x01u;

            if (!bit) continue;   /* transparent: skip '0' bits */

            uint16_t px = (uint16_t)(x + (uint16_t)(col * scale));
            uint16_t py = (uint16_t)(y + (uint16_t)(row * scale));

            if (scale == 1u) {
                ILI9488_DrawPixel(px, py, fg);
            } else {
                ILI9488_FillRect(px, py, scale, scale, fg);
            }
        }
    }
}

void UI_DrawBitmapOpaque(uint16_t x, uint16_t y,
                         const UI_Bitmap_t *bmp,
                         uint8_t scale,
                         uint16_t fg, uint16_t bg)
{
    if (!bmp || !bmp->data) return;

    for (uint8_t row = 0; row < bmp->h; row++) {
        for (uint8_t col = 0; col < bmp->w; col++) {
            uint8_t byte_idx = col / 8u;
            uint8_t bit_pos  = 7u - (col % 8u);
            uint8_t bit = (bmp->data[row * bmp->bpr + byte_idx] >> bit_pos) & 0x01u;

            uint16_t px     = (uint16_t)(x + (uint16_t)(col * scale));
            uint16_t py     = (uint16_t)(y + (uint16_t)(row * scale));
            uint16_t colour = bit ? fg : bg;

            if (scale == 1u) {
                ILI9488_DrawPixel(px, py, colour);
            } else {
                ILI9488_FillRect(px, py, scale, scale, colour);
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   Slider — internal helpers
   ═══════════════════════════════════════════════════════════════════════════ */

/* Map value → knob x pixel */
static uint16_t _ValToKnobX(const UI_Slider_t *s, int16_t val)
{
    int32_t range = (int32_t)(s->val_max - s->val_min);
    if (range == 0) return s->track_x;
    int32_t offset = (int32_t)(val - s->val_min) * (int32_t)s->track_w / range;
    return (uint16_t)(s->track_x + (uint16_t)offset);
}

/* Map touch x pixel → clamped value */
static int16_t _KnobXToVal(const UI_Slider_t *s, uint16_t px)
{
    int32_t offset = (int32_t)px - (int32_t)s->track_x;
    if (offset < 0) offset = 0;
    if (offset > (int32_t)s->track_w) offset = (int32_t)s->track_w;

    int32_t v = s->val_min + offset * (int32_t)(s->val_max - s->val_min)
                             / (int32_t)s->track_w;
    if (v < s->val_min) v = s->val_min;
    if (v > s->val_max) v = s->val_max;
    return (int16_t)v;
}

/* Draw the knob at position kx with full detail */
static void _DrawKnob(const UI_Slider_t *s, uint16_t kx, uint8_t active)
{
    uint16_t knob_col = active ? UI_SLD_COL_FILL : UI_SLD_COL_KNOB;

    /* outer shadow ring (1 px larger, dark) */
    ILI9488_DrawCircle(kx, s->track_y, (uint16_t)(UI_SLIDER_KNOB_R + 1),
                       0x2104u);

    /* filled face */
    ILI9488_FillCircle(kx, s->track_y, UI_SLIDER_KNOB_R, knob_col);

    /* accent border */
    ILI9488_DrawCircle(kx, s->track_y, UI_SLIDER_KNOB_R,
                       UI_SLD_COL_KNOB_BORDER);

    /* inner highlight dot (top-left quadrant) */
    ILI9488_FillCircle((uint16_t)(kx - 3u), (uint16_t)(s->track_y - 3u),
                       2u, UI_SLD_COL_KNOB_DOT);
}

/* Draw the track (filled portion + unfilled portion, no knob) */
static void _DrawTrack(const UI_Slider_t *s, uint16_t kx)
{
    uint16_t ty  = (uint16_t)(s->track_y - UI_SLIDER_TRACK_H / 2u);
    uint8_t  th  = UI_SLIDER_TRACK_H;

    /* track end-caps (rounded look: circle each end) */
    ILI9488_FillCircle(s->track_x,
                       s->track_y,
                       (uint16_t)(UI_SLIDER_TRACK_H / 2u),
                       UI_SLD_COL_TRACK);
    ILI9488_FillCircle((uint16_t)(s->track_x + s->track_w),
                       s->track_y,
                       (uint16_t)(UI_SLIDER_TRACK_H / 2u),
                       UI_SLD_COL_TRACK);

    /* filled (left of knob) */
    if (kx > s->track_x) {
        ILI9488_FillRect(s->track_x, ty,
                         (uint16_t)(kx - s->track_x), th,
                         UI_SLD_COL_FILL);
        /* re-draw left cap in fill colour */
        ILI9488_FillCircle(s->track_x, s->track_y,
                           (uint16_t)(UI_SLIDER_TRACK_H / 2u),
                           UI_SLD_COL_FILL);
    }

    /* unfilled (right of knob) */
    uint16_t track_end = (uint16_t)(s->track_x + s->track_w);
    if (kx < track_end) {
        ILI9488_FillRect(kx, ty,
                         (uint16_t)(track_end - kx), th,
                         UI_SLD_COL_TRACK);
    }
}

/* Draw the value label above the knob */
static void _DrawLabel(const UI_Slider_t *s, uint16_t kx, uint16_t bg_col)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)s->value);

    /* centre the label text above knob (each char is 6px wide at scale 1) */
    uint8_t  len     = (uint8_t)strlen(buf);
    uint16_t text_w  = (uint16_t)(len * 6u);
    uint16_t label_x = (kx > text_w / 2u) ? (uint16_t)(kx - text_w / 2u) : 0u;
    uint16_t label_y = (uint16_t)(s->track_y - UI_SLIDER_KNOB_R - UI_SLIDER_LABEL_H - 4u);

    /* erase label background */
    ILI9488_FillRect((uint16_t)(label_x > 2u ? label_x - 2u : 0u),
                     label_y,
                     (uint16_t)(text_w + 4u),
                     UI_SLIDER_LABEL_H,
                     bg_col);

    ILI9488_DrawString(label_x, label_y, buf,
                       UI_SLD_COL_LABEL_FG, bg_col, 1u);
}

/* Erase the knob bounding box so the track can be redrawn cleanly */
static void _EraseKnob(const UI_Slider_t *s, uint16_t kx, uint16_t bg_col)
{
    uint16_t r = (uint16_t)(UI_SLIDER_KNOB_R + 2u);
    uint16_t ex = (kx > r) ? (uint16_t)(kx - r) : 0u;
    uint16_t ey = (s->track_y > r) ? (uint16_t)(s->track_y - r) : 0u;
    ILI9488_FillRect(ex, ey, (uint16_t)(r * 2u), (uint16_t)(r * 2u), bg_col);
}

/* ═══════════════════════════════════════════════════════════════════════════
   Slider — public API
   ═══════════════════════════════════════════════════════════════════════════ */

void UI_Slider_Init(UI_Slider_t *s,
                    uint16_t track_x, uint16_t track_y, uint16_t track_w,
                    int16_t val_min, int16_t val_max, int16_t initial)
{
    s->track_x   = track_x;
    s->track_y   = track_y;
    s->track_w   = track_w;
    s->val_min   = val_min;
    s->val_max   = val_max;
    s->value     = (initial < val_min) ? val_min
                 : (initial > val_max) ? val_max
                 : initial;
    s->_knob_x   = _ValToKnobX(s, s->value);
    s->_dragging = 0u;
}

void UI_Slider_Draw(UI_Slider_t *s)
{
    uint16_t kx = _ValToKnobX(s, s->value);
    s->_knob_x  = kx;

    _DrawTrack(s, kx);
    _DrawKnob(s, kx, 0u);
    _DrawLabel(s, kx, UI_SLD_COL_LABEL_BG);
}

uint8_t UI_Slider_Touch(UI_Slider_t *s, uint16_t tx, uint16_t ty)
{
    /* Hit test: generous area around knob + track */
    uint16_t hit_r = (uint16_t)(UI_SLIDER_KNOB_R + 8u);
    int16_t  dy    = (int16_t)ty - (int16_t)s->track_y;
    if (dy < 0) dy = -dy;

    uint8_t in_slider = (tx >= (uint16_t)(s->track_x > hit_r
                                          ? s->track_x - hit_r : 0u)) &&
                        (tx <= (uint16_t)(s->track_x + s->track_w + hit_r)) &&
                        ((uint16_t)dy <= hit_r);

    if (!in_slider && !s->_dragging) return 0u;

    s->_dragging = 1u;

    int16_t new_val = _KnobXToVal(s, tx);
    if (new_val == s->value) return 0u;   /* no change */

    /* dirty-rect update -------------------------------------------------- */
    uint16_t old_kx = s->_knob_x;
    s->value        = new_val;
    uint16_t new_kx = _ValToKnobX(s, new_val);
    s->_knob_x      = new_kx;

    /* 1. Erase old knob */
    _EraseKnob(s, old_kx, UI_SLD_COL_LABEL_BG);

    /* 2. Repaint track segment that was under old knob */
    _DrawTrack(s, new_kx);

    /* 3. Draw knob at new position (active style) */
    _DrawKnob(s, new_kx, 1u);

    /* 4. Update floating value label (erase old, draw new) */
    _EraseKnob(s, old_kx, UI_SLD_COL_LABEL_BG);  /* clear old label area too */
    _DrawLabel(s, new_kx, UI_SLD_COL_LABEL_BG);

    return 1u;   /* value changed */
}

void UI_Slider_Release(UI_Slider_t *s)
{
    if (!s->_dragging) return;
    s->_dragging = 0u;
    /* redraw knob in idle style */
    _EraseKnob(s, s->_knob_x, UI_SLD_COL_LABEL_BG);
    _DrawTrack(s, s->_knob_x);
    _DrawKnob(s, s->_knob_x, 0u);
    _DrawLabel(s, s->_knob_x, UI_SLD_COL_LABEL_BG);
}
