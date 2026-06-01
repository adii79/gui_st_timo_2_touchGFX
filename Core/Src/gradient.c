/**
 ******************************************************************************
 * @file    gradient.c
 * @brief   Gradient slider background drawing implementation.
 *
 *          Public API (declared in gradient.h):
 *            DrawStaticSliderImage()        — full scene draw (startup only)
 *            GRADIENT_RedrawSlider2Track()  — partial repaint, slider 2 only
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
#define COL_RED        0xF800u
#define COL_GRAY       0xC618u

#define SCR_W          480u
#define SCR_H          320u

/* ══════════════════════════════════════════════════════════════════════════
   ██  USER-CONFIGURABLE SECTION  ██
   All per-slider geometry lives here. Touch nothing else.

   ORIENTATION   : 0 = horizontal (left→right),  1 = vertical (top→bottom)
   POSITION      : top-left corner of each track in screen pixels
   SIZE          : width × height of each track
   CORNER RADIUS : pill roundness (should be <= half the shorter dimension)

   SATURATION_HUE: background tint hue 0–360.
                   0   = red tint,  120 = green,  240 = blue,  0 = neutral
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider 1 : Rainbow / Hue picker ───────────────────────────────────── */
#define S1_VERTICAL    0
#define S1_X           120u
#define S1_Y           14u
#define S1_W           370u
#define S1_H           32u
#define S1_RX          16u
#define S1_KNOB_X      (S1_X + S1_W - 30u)
#define S1_KNOB_FILL   0xF81Fu    /* magenta */

/* ── Slider 2 : Black → dynamic hue colour ─────────────────────────────── */
#define S2_VERTICAL    0
#define S2_X           120u
#define S2_Y           138u
#define S2_W           370u
#define S2_H           32u
#define S2_RX          16u
#define S2_KNOB_X      (S2_X + S2_W - 10u)
#define S2_KNOB_FILL   0xF800u    /* red (initial) */

/* ── Slider 3 : Alpha → Blue (vertical pill, left side) ────────────────── */
#define S3_VERTICAL    1
#define S3_X           0u
#define S3_Y           0u
#define S3_W           32u
#define S3_H           220u
#define S3_RX          16u
#define S3_KNOB_X      (S3_X + S3_W / 2u)
#define S3_KNOB_FILL   0x001Fu    /* blue */

/* ── Background saturation hue (0–360) ─────────────────────────────────── */
#define SATURATION_HUE   0u       /* 0 = neutral dark charcoal               */

/* ══════════════════════════════════════════════════════════════════════════
   END OF USER-CONFIGURABLE SECTION
   ══════════════════════════════════════════════════════════════════════════ */


/* ══════════════════════════════════════════════════════════════════════════
   GLOBAL STATE
   ══════════════════════════════════════════════════════════════════════════ */

/** Current hue (0–360) driving slider 2's colour ramp. Default = blue. */
uint16_t g_Slider2_Hue = 240u;


/* ══════════════════════════════════════════════════════════════════════════
   HUE → RGB565
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Convert hue (0–360) + saturation + value → RGB565.
 *         Useful for both the background tint and the dynamic slider colour.
 * @param  hue   0–360
 * @param  sat   0–255  saturation
 * @param  val   0–255  brightness
 */
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

/**
 * @brief  Derive the two background gradient colours from SATURATION_HUE.
 */
static void BuildBgColours(uint16_t *bg_top, uint16_t *bg_bot)
{
    *bg_top = HueSVtoRGB565(SATURATION_HUE, 80u,  38u);
    *bg_bot = HueSVtoRGB565(SATURATION_HUE, 100u, 58u);
}


/* ══════════════════════════════════════════════════════════════════════════
   COLOUR MATHS
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Linearly interpolate two RGB565 colours.
 * @param  t   0 = 100 % a,  256 = 100 % b
 */
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
 * @brief  For index i along one axis of a rounded rect, return how many
 *         pixels to clip from each end of the perpendicular axis so the
 *         fill respects the corner radius rx.
 */
static uint16_t RoundRectClip(uint16_t i, uint16_t w,
                               uint16_t h, uint16_t rx)
{
    (void)h;
    uint16_t clip = 0u;

    if (i < rx) {
        uint16_t dx = rx - i;
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
                clip = rx - dy;
                break;
            }
        }
    } else if (i >= w - rx) {
        uint16_t dx = i - (w - rx - 1u);
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
                clip = rx - dy;
                break;
            }
        }
    }
    return clip;
}

/* ── Horizontal gradient fill (left → right) ───────────────────────────── */
static void FillRoundRectGradientH(uint16_t x,  uint16_t y,
                                   uint16_t w,  uint16_t h,
                                   uint16_t rx,
                                   uint16_t (*colFn)(uint16_t, uint16_t))
{
    for (uint16_t i = 0u; i < w; i++) {
        uint16_t clip = RoundRectClip(i, w, h, rx);
        if (clip >= h / 2u) continue;

        uint16_t col = colFn(i, w);
        ILI9488_DrawVLine((uint16_t)(x + i),
                          (uint16_t)(y + clip),
                          (uint16_t)(h - 2u * clip),
                          col);
    }
}

/* ── Vertical gradient fill (top → bottom) ─────────────────────────────── */
static void FillRoundRectGradientV(uint16_t x,  uint16_t y,
                                   uint16_t w,  uint16_t h,
                                   uint16_t rx,
                                   uint16_t (*colFn)(uint16_t, uint16_t))
{
    for (uint16_t j = 0u; j < h; j++) {
        uint16_t col   = colFn(j, h);
        uint16_t hclip = RoundRectClip(j, h, w, rx);
        if (hclip >= w / 2u) continue;

        ILI9488_DrawHLine((uint16_t)(x + hclip),
                          (uint16_t)(y + j),
                          (uint16_t)(w - 2u * hclip),
                          col);
    }
}

/* ── Dispatch: picks H or V based on the vertical flag ─────────────────── */
static void FillRoundRectGradient(uint16_t x,  uint16_t y,
                                  uint16_t w,  uint16_t h,
                                  uint16_t rx,
                                  uint16_t (*colFn)(uint16_t, uint16_t),
                                  uint8_t  vertical)
{
    if (vertical)
        FillRoundRectGradientV(x, y, w, h, rx, colFn);
    else
        FillRoundRectGradientH(x, y, w, h, rx, colFn);
}

/* ── 1-pixel rounded-rect outline ──────────────────────────────────────── */
static void DrawRoundRectOutline(uint16_t x,  uint16_t y,
                                 uint16_t w,  uint16_t h,
                                 uint16_t rx, uint16_t col)
{
    ILI9488_DrawHLine(x + rx,      y,           w - 2u * rx, col);
    ILI9488_DrawHLine(x + rx,      y + h - 1u,  w - 2u * rx, col);
    ILI9488_DrawVLine(x,           y + rx,      h - 2u * rx, col);
    ILI9488_DrawVLine(x + w - 1u, y + rx,      h - 2u * rx, col);

    ILI9488_DrawPixel(x + rx - 1u, y + rx - 1u,  col);
    ILI9488_DrawPixel(x + w - rx,  y + rx - 1u,  col);
    ILI9488_DrawPixel(x + rx - 1u, y + h - rx,   col);
    ILI9488_DrawPixel(x + w - rx,  y + h - rx,   col);
}


/* ══════════════════════════════════════════════════════════════════════════
   COLOUR FUNCTIONS  (one per slider track)
   Signature:  uint16_t fn(uint16_t index, uint16_t total)
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Slider 1 — Full hue rainbow left→right.
 *         red → orange → yellow → green → cyan → blue → magenta
 */
static uint16_t ColFn_Rainbow(uint16_t i, uint16_t w)
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
    uint16_t seg_w = w / 6u;
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
static uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w)
{
    /* Map current hue to a dark midpoint and a bright endpoint */
    uint16_t dark_col   = HueSVtoRGB565(g_Slider2_Hue, 255u, 100u);
    uint16_t bright_col = HueSVtoRGB565(g_Slider2_Hue, 255u, 255u);

    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);

    if (t < 128u) {
        /* Black → dark hue */
        return LerpRGB565(0x0000u, dark_col, (uint16_t)(t * 2u));
    } else {
        /* Dark hue → bright hue */
        return LerpRGB565(dark_col, bright_col, (uint16_t)((t - 128u) * 2u));
    }
}

/**
 * @brief  Slider 3 — Checkerboard (transparency illusion) → solid blue.
 *         Left 40 %: alternating 8-px checker columns.
 *         Right 60 %: smooth blend from checker tone to pure blue.
 */
static uint16_t ColFn_AlphaToBlue(uint16_t i, uint16_t w)
{
    static const uint16_t CHK_LIGHT = 0xEF7Bu;
    static const uint16_t CHK_DARK  = 0xCE79u;
    static const uint16_t BLUE      = 0x001Fu;

    uint16_t cross = (uint16_t)(((uint32_t)w * 40u) / 100u);

    if (i < cross) {
        return ((i / 8u) & 1u) ? CHK_DARK : CHK_LIGHT;
    } else {
        uint16_t t = (uint16_t)(((uint32_t)(i - cross) * 256u) / (w - cross));
        return LerpRGB565(CHK_DARK, BLUE, t);
    }
}


/* ══════════════════════════════════════════════════════════════════════════
   KNOB DRAWING  (polished style — not currently used, kept for reference)
   ══════════════════════════════════════════════════════════════════════════ */
#if 0
static void DrawSliderKnob(uint16_t x, uint16_t y, uint16_t fill)
{
    ILI9488_FillCircle((uint16_t)(x + 2u), (uint16_t)(y + 2u), 12u, 0xBDF7u);
    ILI9488_FillCircle(x, y, 12u, COL_WHITE);
    ILI9488_DrawCircle(x, y, 12u, COL_GRAY);
    ILI9488_FillCircle(x, y,  8u, fill);
    ILI9488_FillCircle((uint16_t)(x - 3u), (uint16_t)(y - 3u), 3u, COL_WHITE);
}
#endif


/* ══════════════════════════════════════════════════════════════════════════
   SLIDER GEOMETRY TABLE
   Shared by both DrawStaticSliderImage and GRADIENT_RedrawSlider2Track.
   Declared static const so it lives in flash.
   ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint16_t  x, y, w, h, rx;             /* track geometry                */
    uint8_t   vertical;                    /* 0 = horizontal, 1 = vertical  */
    uint16_t (*fn)(uint16_t, uint16_t);    /* colour function               */
} slider_def_t;

static const slider_def_t s_sliders[3] = {
    /* Slider 1 — Rainbow hue picker */
    { S1_X, S1_Y, S1_W, S1_H, S1_RX, S1_VERTICAL, ColFn_Rainbow    },
    /* Slider 2 — Black → dynamic hue */
    { S2_X, S2_Y, S2_W, S2_H, S2_RX, S2_VERTICAL, ColFn_BlackToRed },
    /* Slider 3 — Alpha → Blue */
    { S3_X, S3_Y, S3_W, S3_H, S3_RX, S3_VERTICAL, ColFn_AlphaToBlue},
};


/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC: DrawStaticSliderImage
   Full scene draw — call once on startup after UGFX_Commit().
   ══════════════════════════════════════════════════════════════════════════ */
void DrawStaticSliderImage(void)
{
    /* ── 1. Full-screen dark gradient background ─────────────────────────
       One horizontal line per scanline using the hue-tinted palette.      */
    uint16_t bg_top, bg_bot;
    BuildBgColours(&bg_top, &bg_bot);

    for (uint16_t y = 0u; y < SCR_H; y++) {
        uint16_t t   = (uint16_t)(((uint32_t)y * 256u) / SCR_H);
        uint16_t col = LerpRGB565(bg_top, bg_bot, t);
        ILI9488_DrawHLine(0u, y, SCR_W, col);
    }

    /* ── 2. All three slider track pills ────────────────────────────────── */
    for (uint8_t r = 0u; r < 3u; r++) {
        FillRoundRectGradient(s_sliders[r].x, s_sliders[r].y,
                              s_sliders[r].w, s_sliders[r].h,
                              s_sliders[r].rx,
                              s_sliders[r].fn,
                              s_sliders[r].vertical);

        DrawRoundRectOutline(s_sliders[r].x, s_sliders[r].y,
                             s_sliders[r].w, s_sliders[r].h,
                             s_sliders[r].rx, COL_WHITE);
    }
}


/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC: GRADIENT_RedrawSlider2Track
   Partial repaint — slider 2 pill ONLY, no background, no other sliders.
   Call after writing g_Slider2_Hue for a zero-flicker live colour update.
   ══════════════════════════════════════════════════════════════════════════ */
void GRADIENT_RedrawSlider2Track(void)
{
    const slider_def_t *s = &s_sliders[1];   /* index 1 = slider 2 */

    FillRoundRectGradient(s->x, s->y, s->w, s->h,
                          s->rx, s->fn, s->vertical);

    DrawRoundRectOutline(s->x, s->y, s->w, s->h,
                         s->rx, COL_WHITE);
}