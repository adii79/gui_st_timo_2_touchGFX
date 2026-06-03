/**
 ******************************************************************************
 * @file    gradient.c
 * @brief   Gradient slider background drawing implementation.
 *
 *          Public API (declared in gradient.h):
 *            DrawStaticSliderImage()        — full scene draw (startup only)
 *            GRADIENT_RedrawSlider1Track()  — repaint slider 1 pill only
 *            GRADIENT_RedrawSlider2Track()  — repaint slider 2 pill only
 *            GRADIENT_RedrawSlider3Track()  — repaint slider 3 pill only
 *
 *  HOW THE GRADIENT-BEHIND-KNOB WORKS
 *  ────────────────────────────────────
 *  Each ugfx_slider_t has a  bg_redraw_fn  field (void(*)(void)).
 *  UGFX_SliderDraw() calls it immediately after erasing the old knob and
 *  before drawing the new knob, so the gradient pill is always restored
 *  underneath the bitmap knob with zero flicker.
 *
 *  Wire-up in main.c (once, after the builder .build() call):
 *      g_sliderR->bg_redraw_fn = GRADIENT_RedrawSlider1Track;
 *      g_sliderG->bg_redraw_fn = GRADIENT_RedrawSlider2Track;
 *      g_sliderB->bg_redraw_fn = GRADIENT_RedrawSlider3Track;
 ******************************************************************************
 */

#include "gradient.h"
#include "main.h"
#include "ili9488.h"

/* ══════════════════════════════════════════════════════════════════════════
   RGB565 COLOUR CONSTANTS
   ══════════════════════════════════════════════════════════════════════════ */
#define COL_WHITE      0xFFFFu
#define COL_BLACK      0x0000u

#define SCR_W          480u
#define SCR_H          320u

/* ══════════════════════════════════════════════════════════════════════════
   ██  USER-CONFIGURABLE GEOMETRY  ██
   These MUST match the ugfx slider .origin() / .frame() calls in main.c
   so the gradient pill sits exactly behind the interactive knob.

   Slider 1 (VERTICAL)   — origin(75,  40),  frame(30,  150)
   Slider 2 (HORIZONTAL) — origin(125, 60),  frame(360,  15)
   Slider 3 (HORIZONTAL) — origin(125, 185), frame(360,  15)
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider 1 : Rainbow / Hue picker (VERTICAL pill) ───────────────────── */
#define S1_VERTICAL    0
#define S1_X           100u
#define S1_Y           35u
#define S1_W           360u
#define S1_H           60u
#define S1_RX          0u        /* = S1_W/2 → perfect pill ends           */

/* ── Slider 2 : Black → dynamic hue (HORIZONTAL pill) ──────────────────── */
#define S2_VERTICAL    0
#define S2_X           100u
#define S2_Y           140u
#define S2_W           360u
#define S2_H           60u
#define S2_RX           0u

/* ── Slider 3 : Checkerboard → Blue (HORIZONTAL pill) ──────────────────── */
#define S3_VERTICAL    1
#define S3_X           25u
#define S3_Y           35u
#define S3_W           60u
#define S3_H           250u
#define S3_RX           0u

/* ── Background saturation hue (0–360).  0 = neutral dark charcoal ──────── */
#define SATURATION_HUE   0u

/* ══════════════════════════════════════════════════════════════════════════
   GLOBAL STATE
   ══════════════════════════════════════════════════════════════════════════ */

uint16_t g_Slider2_Hue = 240u;   /* default hue = blue */

/* ══════════════════════════════════════════════════════════════════════════
   HUE → RGB565
   ══════════════════════════════════════════════════════════════════════════ */

static uint16_t HueSVtoRGB565(uint16_t hue, uint8_t sat, uint8_t val)
{
    uint32_t h      = ((uint32_t)hue * 1530u) / 360u;
    uint32_t s      = sat;
    uint32_t v      = val;
    uint32_t sector = h / 255u;
    uint32_t frac   = h % 255u;

    uint32_t p  = (v * (255u - s)) / 255u;
    uint32_t q  = (v * (255u - (s * frac)         / 255u)) / 255u;
    uint32_t tv = (v * (255u - (s * (255u - frac)) / 255u)) / 255u;

    uint32_t r, g, b;
    switch (sector % 6u) {
        case 0u:  r = v;  g = tv; b = p;  break;
        case 1u:  r = q;  g = v;  b = p;  break;
        case 2u:  r = p;  g = v;  b = tv; break;
        case 3u:  r = p;  g = q;  b = v;  break;
        case 4u:  r = tv; g = p;  b = v;  break;
        default:  r = v;  g = p;  b = q;  break;
    }
    return (uint16_t)(((r >> 3u) << 11u) | ((g >> 2u) << 5u) | (b >> 3u));
}

static void BuildBgColours(uint16_t *top, uint16_t *bot)
{
    *top = HueSVtoRGB565(SATURATION_HUE, 80u,  38u);
    *bot = HueSVtoRGB565(SATURATION_HUE, 100u, 58u);
}

/* ══════════════════════════════════════════════════════════════════════════
   COLOUR MATHS
   ══════════════════════════════════════════════════════════════════════════ */

uint16_t LerpRGB565(uint16_t a, uint16_t b, uint16_t t)
{
    uint8_t r  = (uint8_t)(((uint16_t)((a >> 11) & 0x1Fu) * (256u - t)
                           + (uint16_t)((b >> 11) & 0x1Fu) * t) >> 8u);
    uint8_t g  = (uint8_t)(((uint16_t)((a >>  5) & 0x3Fu) * (256u - t)
                           + (uint16_t)((b >>  5) & 0x3Fu) * t) >> 8u);
    uint8_t bl = (uint8_t)(((uint16_t)( a         & 0x1Fu) * (256u - t)
                           + (uint16_t)( b         & 0x1Fu) * t) >> 8u);
    return (uint16_t)((r << 11u) | (g << 5u) | bl);
}

/* ══════════════════════════════════════════════════════════════════════════
   ROUNDED-RECT HELPERS
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  How many pixels to clip from each end of the perpendicular axis
 *         at position i along the primary axis, for a rounded corner of rx.
 */
static uint16_t RoundRectClip(uint16_t i, uint16_t total, uint16_t rx)
{
    uint16_t clip = 0u;
    if (i < rx) {
        uint16_t dx = rx - i;
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
                clip = rx - dy;
                break;
            }
        }
    } else if (i >= total - rx) {
        uint16_t dx = i - (total - rx - 1u);
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
                clip = rx - dy;
                break;
            }
        }
    }
    return clip;
}

/* ── Horizontal gradient pill (colour varies left → right) ─────────────── */
static void FillRoundRectGradientH(uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h, uint16_t rx,
                                   uint16_t (*fn)(uint16_t, uint16_t))
{
    for (uint16_t i = 0u; i < w; i++) {
        uint16_t clip = RoundRectClip(i, w, rx);
        if (clip >= h / 2u) continue;
        uint16_t col = fn(i, w);
        ILI9488_DrawVLine((uint16_t)(x + i),
                          (uint16_t)(y + clip),
                          (uint16_t)(h - 2u * clip), col);
    }
}

/* ── Vertical gradient pill (colour varies top → bottom) ───────────────── */
static void FillRoundRectGradientV(uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h, uint16_t rx,
                                   uint16_t (*fn)(uint16_t, uint16_t))
{
    for (uint16_t j = 0u; j < h; j++) {
        uint16_t clip = RoundRectClip(j, h, rx);
        if (clip >= w / 2u) continue;
        uint16_t col = fn(j, h);
        ILI9488_DrawHLine((uint16_t)(x + clip),
                          (uint16_t)(y + j),
                          (uint16_t)(w - 2u * clip), col);
    }
}

static void FillRoundRectGradient(uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h, uint16_t rx,
                                  uint16_t (*fn)(uint16_t, uint16_t),
                                  uint8_t vertical)
{
    if (vertical)
        FillRoundRectGradientV(x, y, w, h, rx, fn);
    else
        FillRoundRectGradientH(x, y, w, h, rx, fn);
}

/* ── 1-pixel rounded-rect outline ──────────────────────────────────────── */
static void DrawRoundRectOutline(uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h,
                                 uint16_t rx, uint16_t col)
{
    ILI9488_DrawHLine(x + rx,     y,          w - 2u * rx, col);
    ILI9488_DrawHLine(x + rx,     y + h - 1u, w - 2u * rx, col);
    ILI9488_DrawVLine(x,          y + rx,     h - 2u * rx, col);
    ILI9488_DrawVLine(x + w - 1u, y + rx,     h - 2u * rx, col);

    /* Corner pixel approximation */
    ILI9488_DrawPixel(x + rx - 1u, y + rx - 1u, col);
    ILI9488_DrawPixel(x + w - rx,  y + rx - 1u, col);
    ILI9488_DrawPixel(x + rx - 1u, y + h - rx,  col);
    ILI9488_DrawPixel(x + w - rx,  y + h - rx,  col);
}

/* ══════════════════════════════════════════════════════════════════════════
   COLOUR FUNCTIONS  (one per slider track)
   Signature: uint16_t fn(uint16_t index, uint16_t total)
   index counts along the primary gradient axis (H: left→right, V: top→bot).
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Slider 1 — Full hue rainbow.
 *         Vertical pill: top = red → bottom = magenta (full spectrum).
 */
static uint16_t ColFn_Rainbow(uint16_t i, uint16_t total)
{
    static const uint16_t stops[7] = {
        0xF800u,   /* red     */
        0xFD20u,   /* orange  */
        0xFFE0u,   /* yellow  */
        0x07E0u,   /* green   */
        0x07FFu,   /* cyan    */
        0x001Fu,   /* blue    */
        0xF81Fu,   /* magenta */
    };
    uint16_t seg_w = total / 6u;
    if (seg_w == 0u) seg_w = 1u;
    uint16_t seg = i / seg_w;
    if (seg >= 6u) seg = 5u;
    uint16_t t = (uint16_t)(((uint32_t)(i - seg * seg_w) * 256u) / seg_w);
    return LerpRGB565(stops[seg], stops[seg + 1u], t);
}

/**
 * @brief  Slider 2 — Black → dark hue → bright hue.
 *         Colour is derived from the global g_Slider2_Hue (set by slider 1).
 */
static uint16_t ColFn_BlackToHue(uint16_t i, uint16_t total)
{
    uint16_t dark_col   = HueSVtoRGB565(g_Slider2_Hue, 255u, 100u);
    uint16_t bright_col = HueSVtoRGB565(g_Slider2_Hue, 255u, 255u);

    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / total);
    if (t < 128u)
        return LerpRGB565(0x0000u, dark_col, (uint16_t)(t * 2u));
    else
        return LerpRGB565(dark_col, bright_col, (uint16_t)((t - 128u) * 2u));
}

/**
 * @brief  Slider 3 — Checkerboard (transparency illusion) → solid blue.
 *         Left 40 %: alternating 8-px checker columns.
 *         Right 60 %: smooth blend checker tone → pure blue.
 */
static uint16_t ColFn_AlphaToBlue(uint16_t i, uint16_t total)
{
    static const uint16_t CHK_LIGHT = 0xEF7Bu;
    static const uint16_t CHK_DARK  = 0xCE79u;
    static const uint16_t BLUE      = 0x001Fu;

    uint16_t cross = (uint16_t)(((uint32_t)total * 40u) / 100u);
    if (i < cross) {
        return ((i / 8u) & 1u) ? CHK_DARK : CHK_LIGHT;
    } else {
        uint16_t t = (uint16_t)(((uint32_t)(i - cross) * 256u) / (total - cross));
        return LerpRGB565(CHK_DARK, BLUE, t);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   SLIDER GEOMETRY TABLE  (flash; shared by all draw calls)
   ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint16_t  x, y, w, h, rx;
    uint8_t   vertical;
    uint16_t (*fn)(uint16_t, uint16_t);
} slider_def_t;

static const slider_def_t s_sliders[3] = {
    /* 0 — Rainbow hue picker (VERTICAL) */
    { S1_X, S1_Y, S1_W, S1_H, S1_RX, S1_VERTICAL, ColFn_Rainbow    },
    /* 1 — Black → dynamic hue (HORIZONTAL) */
    { S2_X, S2_Y, S2_W, S2_H, S2_RX, S2_VERTICAL, ColFn_BlackToHue },
    /* 2 — Checkerboard → Blue (HORIZONTAL) */
    { S3_X, S3_Y, S3_W, S3_H, S3_RX, S3_VERTICAL, ColFn_AlphaToBlue},
};

/* ══════════════════════════════════════════════════════════════════════════
   INTERNAL HELPER — draw one pill by index
   ══════════════════════════════════════════════════════════════════════════ */

static void _DrawPill(uint8_t idx)
{
    const slider_def_t *s = &s_sliders[idx];
    FillRoundRectGradient(s->x, s->y, s->w, s->h,
                          s->rx, s->fn, s->vertical);
    DrawRoundRectOutline (s->x, s->y, s->w, s->h,
                          s->rx, COL_WHITE);
}

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC: DrawStaticSliderImage
   Full scene draw — call once BEFORE UGFX_Commit() on startup.
   ══════════════════════════════════════════════════════════════════════════ */
void DrawStaticSliderImage(void)
{
    /* 1. Full-screen dark gradient background */
    uint16_t bg_top, bg_bot;
    BuildBgColours(&bg_top, &bg_bot);
    for (uint16_t y = 0u; y < SCR_H; y++) {
        uint16_t t   = (uint16_t)(((uint32_t)y * 256u) / SCR_H);
        uint16_t col = LerpRGB565(bg_top, bg_bot, t);
        ILI9488_DrawHLine(0u, y, SCR_W, col);
    }

    /* 2. Three slider track pills */
    _DrawPill(0u);
    _DrawPill(1u);
    _DrawPill(2u);
}

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC: Per-slider partial repaints
   ══════════════════════════════════════════════════════════════════════════ */
void GRADIENT_RedrawSlider1Track(void) { _DrawPill(0u); }
void GRADIENT_RedrawSlider2Track(void) { _DrawPill(1u); }
void GRADIENT_RedrawSlider3Track(void) { _DrawPill(2u); }
