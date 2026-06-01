////
////#include "gradient.h"
////#include "main.h"
////#include "ili9488.h"
////
////
/////* USER CODE END PV */
////
////
/////* USER CODE BEGIN PFP */
////
/////* ══════════════════════════════════════════════════════════════════════════
////   RGB565 COLOUR CONSTANTS
////   ══════════════════════════════════════════════════════════════════════════ */
////#define COL_WHITE      0xFFFFu
////#define COL_BLACK      0x0000u
////#define COL_RED        0xF800u
////#define COL_GRAY       0xC618u
////
/////* Dark gradient palette for the main background */
////#define BG_TOP         0x10A2u   /* very dark blue-charcoal (top of screen)    */
////#define BG_BOT         0x2965u   /* slightly lighter slate  (bottom of screen) */
////
/////* Screen dimensions (landscape) */
////#define SCR_W          480u
////#define SCR_H          320u
////
/////* ══════════════════════════════════════════════════════════════════════════
////   SLIDER TRACK GEOMETRY
////   Three horizontal sliders, stacked vertically.
////   ══════════════════════════════════════════════════════════════════════════ */
////#define TRACK_X        120u    /* left edge of all three tracks                 */
////#define TRACK_W        370u   /* track width                                   */
////#define TRACK_H        32u    /* track height                                  */
////#define TRACK_RX       16u    /* corner radius for rounded pill shape          */
////#define ROW_Y1         30u    /* centre-y of row 1  (rainbow / hue slider)     */
////#define ROW_Y2         154u   /* centre-y of row 2  (black→red slider)         */
////#define ROW_Y3         264u   /* centre-y of row 3  (alpha→blue slider)        */
////
/////* ══════════════════════════════════════════════════════════════════════════
////   COLOUR MATHS
////   ══════════════════════════════════════════════════════════════════════════ */
////
/////**
//// * @brief  Linearly interpolate two RGB565 colours.
//// * @param  a   start colour
//// * @param  b   end colour
//// * @param  t   0 = 100% a,  256 = 100% b
//// * @return     interpolated RGB565
//// */
//// uint16_t LerpRGB565(uint16_t a, uint16_t b, uint16_t t)
////{
////    uint8_t r  = (uint8_t)(((uint16_t)((a >> 11) & 0x1Fu) * (256u - t)
////                           + (uint16_t)((b >> 11) & 0x1Fu) * t) >> 8u);
////    uint8_t g  = (uint8_t)(((uint16_t)((a >>  5) & 0x3Fu) * (256u - t)
////                           + (uint16_t)((b >>  5) & 0x3Fu) * t) >> 8u);
////    uint8_t bl = (uint8_t)(((uint16_t)( a         & 0x1Fu) * (256u - t)
////                           + (uint16_t)( b         & 0x1Fu) * t) >> 8u);
////    return (uint16_t)((r << 11u) | (g << 5u) | bl);
////}
////
/////* ══════════════════════════════════════════════════════════════════════════
////   ROUNDED-RECT HELPERS
////   ══════════════════════════════════════════════════════════════════════════ */
////
/////**
//// * @brief  For column i inside a rounded rect of width w and corner radius rx,
//// *         return how many pixels to clip from the top (and symmetrically from
//// *         the bottom) so the fill respects the rounded corners.
//// */
//// uint16_t RoundRectClip(uint16_t i, uint16_t w,
////                               uint16_t h, uint16_t rx)
////{
////    (void)h;   /* symmetrical — only rx matters */
////
////    uint16_t clip = 0u;
////
////    if (i < rx) {
////        /* left corner: dx = distance from corner centre to this column */
////        uint16_t dx = rx - i;
////        /* find largest dy such that dx²+dy² <= rx² */
////        for (uint16_t dy = 0u; dy < rx; dy++) {
////            if ((uint32_t)dx * dx + (uint32_t)dy * dy
////                    <= (uint32_t)rx * rx) {
////                clip = rx - dy;
////                break;
////            }
////        }
////    } else if (i >= w - rx) {
////        /* right corner */
////        uint16_t dx = i - (w - rx - 1u);
////        for (uint16_t dy = 0u; dy < rx; dy++) {
////            if ((uint32_t)dx * dx + (uint32_t)dy * dy
////                    <= (uint32_t)rx * rx) {
////                clip = rx - dy;
////                break;
////            }
////        }
////    }
////
////    return clip;
////}
////
/////**
//// * @brief  Fill a rounded rect column by column, calling colFn(i, w) to
//// *         obtain the colour for each column i (0 = left-most).
//// *         colFn signature:  uint16_t fn(uint16_t col_idx, uint16_t total_w)
//// */
//// void FillRoundRectGradientH(uint16_t x,  uint16_t y,
////                                    uint16_t w,  uint16_t h,
////                                    uint16_t rx,
////                                    uint16_t (*colFn)(uint16_t, uint16_t))
////{
////    for (uint16_t i = 0u; i < w; i++) {
////        uint16_t clip = RoundRectClip(i, w, h, rx);
////        if (clip >= h / 2u) continue;          /* fully outside rounded corner */
////
////        uint16_t col = colFn(i, w);
////        ILI9488_DrawVLine((uint16_t)(x + i),
////                          (uint16_t)(y + clip),
////                          (uint16_t)(h - 2u * clip),
////                          col);
////    }
////}
////
/////**
//// * @brief  Draw a 1-pixel rounded-rect outline.
//// */
//// void DrawRoundRectOutline(uint16_t x,  uint16_t y,
////                                  uint16_t w,  uint16_t h,
////                                  uint16_t rx, uint16_t col)
////{
////    /* straight edges */
////    ILI9488_DrawHLine(x + rx,        y,          w - 2u * rx, col);
////    ILI9488_DrawHLine(x + rx,        y + h - 1u, w - 2u * rx, col);
////    ILI9488_DrawVLine(x,             y + rx,     h - 2u * rx, col);
////    ILI9488_DrawVLine(x + w - 1u,   y + rx,     h - 2u * rx, col);
////
////    /* single-pixel corner bevels (good enough for this display size) */
////    ILI9488_DrawPixel(x + rx - 1u,  y + rx - 1u,          col);
////    ILI9488_DrawPixel(x + w - rx,   y + rx - 1u,          col);
////    ILI9488_DrawPixel(x + rx - 1u,  y + h - rx,           col);
////    ILI9488_DrawPixel(x + w - rx,   y + h - rx,           col);
////}
////
/////* ══════════════════════════════════════════════════════════════════════════
////   COLOUR FUNCTIONS  (one per slider track)
////   Signature:  uint16_t fn(uint16_t col_idx, uint16_t total_width)
////   ══════════════════════════════════════════════════════════════════════════ */
////
/////**
//// * @brief  Row 1 — Full hue rainbow: red→orange→yellow→green→cyan→blue→magenta
//// */
//// uint16_t ColFn_Rainbow(uint16_t i, uint16_t w)
////{
////    static const uint16_t stops[7] = {
////        0xF800u,   /* red     */
////        0xFD20u,   /* orange  */
////        0xFFE0u,   /* yellow  */
////        0x07E0u,   /* green   */
////        0x07FFu,   /* cyan    */
////        0x001Fu,   /* blue    */
////        0xF81Fu,   /* magenta */
////    };
////    /* divide the track into 6 equal segments */
////    uint16_t seg_w = w / 6u;
////    if (seg_w == 0u) seg_w = 1u;
////    uint16_t seg   = i / seg_w;
////    if (seg >= 6u) seg = 5u;
////    uint16_t t     = (uint16_t)(((uint32_t)(i - seg * seg_w) * 256u) / seg_w);
////    return LerpRGB565(stops[seg], stops[seg + 1u], t);
////}
////
/////**
//// * @brief  Row 2 — Black → dark red → bright red  (two-stage ramp)
//// */
//// uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w)
////{
////    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);
////    if (t < 128u) {
////        /* 0x0000 → 0x6000  (very dark red midpoint) */
////        return LerpRGB565(0x0000u, 0x6000u, (uint16_t)(t * 2u));
////    } else {
////        /* 0x6000 → 0xF800  (full red) */
////        return LerpRGB565(0x6000u, 0xF800u, (uint16_t)((t - 128u) * 2u));
////    }
////}
////
/////**
//// * @brief  Row 3 — Checkerboard (transparency illusion) → solid blue
//// *
//// *         Left 40 % : alternating 8-pixel columns of two checker grays
//// *         Right 60 % : smooth blend from the checker-gray tone to pure blue
//// */
//// uint16_t ColFn_AlphaToBlue(uint16_t i, uint16_t w)
////{
////    static const uint16_t CHK_LIGHT = 0xEF7Bu;   /* light checker gray  */
////    static const uint16_t CHK_DARK  = 0xCE79u;   /* dark  checker gray  */
////    static const uint16_t BLUE      = 0x001Fu;   /* pure blue           */
////
////    uint16_t cross = (uint16_t)(((uint32_t)w * 40u) / 100u);
////
////    if (i < cross) {
////        /* checkerboard: 8-pixel-wide alternating columns */
////        return ((i / 8u) & 1u) ? CHK_DARK : CHK_LIGHT;
////    } else {
////        /* blend from the average checker tone → pure blue */
////        uint16_t t = (uint16_t)(((uint32_t)(i - cross) * 256u) / (w - cross));
////        return LerpRGB565(CHK_DARK, BLUE, t);
////    }
////}
////
/////* ══════════════════════════════════════════════════════════════════════════
////   KNOB DRAWING  (same polished style as original)
////   ══════════════════════════════════════════════════════════════════════════ */
////
//// void DrawSliderKnob(uint16_t x, uint16_t y, uint16_t fill)
////{
////    /* drop shadow */
////    ILI9488_FillCircle((uint16_t)(x + 2u), (uint16_t)(y + 2u), 12u, 0xBDF7u);
////    /* white outer ring */
////    ILI9488_FillCircle(x, y, 12u, COL_WHITE);
////    /* thin border */
////    ILI9488_DrawCircle(x, y, 12u, COL_GRAY);
////    /* coloured centre */
////    ILI9488_FillCircle(x, y, 8u, fill);
////    /* specular highlight */
////    ILI9488_FillCircle((uint16_t)(x - 3u), (uint16_t)(y - 3u), 3u, COL_WHITE);
////}
////
/////* ═════════════════════════════════════════════════════════a═════════════════
////   MAIN SCENE DRAW
////   Called once after UGFX_Commit() to paint the decorative background
////   and the three gradient slider tracks.
////   ══════════════════════════════════════════════════════════════════════════ */
////
//// void DrawStaticSliderImage(void)
////{
////    /* ── 1. Top-to-bottom dark gradient background ───────────────────────
////       One horizontal line per scanline — fast on SPI because FillRect
////       is internally batched.                                               */
////    for (uint16_t y = 0u; y < SCR_H; y++) {
////        uint16_t t   = (uint16_t)(((uint32_t)y * 256u) / SCR_H);
////        uint16_t col = LerpRGB565(BG_TOP, BG_BOT, t);
////        ILI9488_DrawHLine(0u, y, SCR_W, col);
////    }
////
////    /* ── 2. Three gradient slider track backgrounds ──────────────────────
////       Each track is a pill-shaped rounded rect filled column-by-column
////       with a unique colour function, topped with a white outline.         */
////
////    static const struct {
////        uint16_t  cy;                                  /* row centre-y     */
////        uint16_t (*fn)(uint16_t, uint16_t);            /* colour function  */
////        uint16_t  knob_x;                              /* knob x position  */
////        uint16_t  knob_fill;                           /* knob fill colour */
////    } rows[3] = {
////        { ROW_Y1, ColFn_Rainbow,    (uint16_t)(TRACK_X + TRACK_W - 30u), 0xF81Fu },  /* magenta knob, far right  */
////        { ROW_Y2, ColFn_BlackToRed, (uint16_t)(TRACK_X + TRACK_W - 10u), 0xF800u },  /* red knob,     far right  */
////        { ROW_Y3, ColFn_AlphaToBlue,(uint16_t)(TRACK_X + TRACK_W / 2u),  0x001Fu },  /* blue knob,    middle     */
////    };
////
////    for (uint8_t r = 0u; r < 3u; r++) {
////        uint16_t ty = (uint16_t)(rows[r].cy - TRACK_H / 2u);
////
////        /* gradient fill */
////        FillRoundRectGradientH(TRACK_X, ty, TRACK_W, TRACK_H,
////                               TRACK_RX, rows[r].fn);
////
////        /* white outline */
////        DrawRoundRectOutline(TRACK_X, ty, TRACK_W, TRACK_H,
////                             TRACK_RX, COL_WHITE);
////
////        /* knob */
////        // DrawSliderKnob(rows[r].knob_x,
////        //                rows[r].cy,
////        //                rows[r].knob_fill);
////    }
////}
//
//
//
//
//
//
//#include "gradient.h"
//#include "main.h"
//#include "ili9488.h"
//
///* ══════════════════════════════════════════════════════════════════════════
//   RGB565 COLOUR CONSTANTS
//   ══════════════════════════════════════════════════════════════════════════ */
//#define COL_WHITE      0xFFFFu
//#define COL_BLACK      0x0000u
//#define COL_RED        0xF800u
//#define COL_GRAY       0xC618u
//
//#define BG_TOP         0x10A2u
//#define BG_BOT         0x2965u
//
//#define SCR_W          480u
//#define SCR_H          320u
//
///* ══════════════════════════════════════════════════════════════════════════
//   SLIDER TRACK GEOMETRY
//   ══════════════════════════════════════════════════════════════════════════ */
//#define TRACK_X        120u
//#define TRACK_W        370u
//#define TRACK_H        32u
//#define TRACK_RX       16u
//#define ROW_Y1         30u
//#define ROW_Y2         154u
//#define ROW_Y3         264u
//
///* ══════════════════════════════════════════════════════════════════════════
//   PER-SLIDER ORIENTATION SELECTION  (edit here — no changes needed elsewhere)
//   0 = horizontal gradient (left → right)
//   1 = vertical   gradient (top  → bottom)
//   ══════════════════════════════════════════════════════════════════════════ */
//#define SLIDER1_VERTICAL   0   /* Rainbow         slider */
//#define SLIDER2_VERTICAL   0   /* Black→Red       slider */
//#define SLIDER3_VERTICAL   1   /* Alpha→Blue      slider */
//
///* ══════════════════════════════════════════════════════════════════════════
//   COLOUR MATHS
//   ══════════════════════════════════════════════════════════════════════════ */
//uint16_t LerpRGB565(uint16_t a, uint16_t b, uint16_t t)
//{
//    uint8_t r  = (uint8_t)(((uint16_t)((a >> 11) & 0x1Fu) * (256u - t)
//                           + (uint16_t)((b >> 11) & 0x1Fu) * t) >> 8u);
//    uint8_t g  = (uint8_t)(((uint16_t)((a >>  5) & 0x3Fu) * (256u - t)
//                           + (uint16_t)((b >>  5) & 0x3Fu) * t) >> 8u);
//    uint8_t bl = (uint8_t)(((uint16_t)( a         & 0x1Fu) * (256u - t)
//                           + (uint16_t)( b         & 0x1Fu) * t) >> 8u);
//    return (uint16_t)((r << 11u) | (g << 5u) | bl);
//}
//
///* ══════════════════════════════════════════════════════════════════════════
//   ROUNDED-RECT HELPERS
//   ══════════════════════════════════════════════════════════════════════════ */
//uint16_t RoundRectClip(uint16_t i, uint16_t w, uint16_t h, uint16_t rx)
//{
//    (void)h;
//    uint16_t clip = 0u;
//
//    if (i < rx) {
//        uint16_t dx = rx - i;
//        for (uint16_t dy = 0u; dy < rx; dy++) {
//            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
//                clip = rx - dy;
//                break;
//            }
//        }
//    } else if (i >= w - rx) {
//        uint16_t dx = i - (w - rx - 1u);
//        for (uint16_t dy = 0u; dy < rx; dy++) {
//            if ((uint32_t)dx * dx + (uint32_t)dy * dy <= (uint32_t)rx * rx) {
//                clip = rx - dy;
//                break;
//            }
//        }
//    }
//    return clip;
//}
//
///* ── Horizontal gradient fill (original behaviour) ──────────────────────── */
//static void FillRoundRectGradientH(uint16_t x,  uint16_t y,
//                                   uint16_t w,  uint16_t h,
//                                   uint16_t rx,
//                                   uint16_t (*colFn)(uint16_t, uint16_t))
//{
//    for (uint16_t i = 0u; i < w; i++) {
//        uint16_t clip = RoundRectClip(i, w, h, rx);
//        if (clip >= h / 2u) continue;
//
//        uint16_t col = colFn(i, w);
//        ILI9488_DrawVLine((uint16_t)(x + i),
//                          (uint16_t)(y + clip),
//                          (uint16_t)(h - 2u * clip),
//                          col);
//    }
//}
//
///* ── Vertical gradient fill (top→bottom, colour keyed on row within track) ─ */
//static void FillRoundRectGradientV(uint16_t x,  uint16_t y,
//                                   uint16_t w,  uint16_t h,
//                                   uint16_t rx,
//                                   uint16_t (*colFn)(uint16_t, uint16_t))
//{
//    for (uint16_t j = 0u; j < h; j++) {
//        /* For the vertical case we reuse colFn with (row_index, total_height).
//           This maps the same colour ramp onto the vertical axis instead.    */
//        uint16_t col = colFn(j, h);
//
//        /* Compute the horizontal span at this row accounting for rounded corners.
//           We repurpose RoundRectClip: treat the row index as a "column index"
//           along the shorter (height) axis.                                  */
//        uint16_t hclip = RoundRectClip(j, h, w, rx);
//        if (hclip >= w / 2u) continue;
//
//        ILI9488_DrawHLine((uint16_t)(x + hclip),
//                          (uint16_t)(y + j),
//                          (uint16_t)(w - 2u * hclip),
//                          col);
//    }
//}
//
///* ── Dispatch wrapper: picks H or V at compile-time per slider ───────────── */
//static void FillRoundRectGradient(uint16_t x,   uint16_t y,
//                                  uint16_t w,   uint16_t h,
//                                  uint16_t rx,
//                                  uint16_t (*colFn)(uint16_t, uint16_t),
//                                  uint8_t  vertical)
//{
//    if (vertical) {
//        FillRoundRectGradientV(x, y, w, h, rx, colFn);
//    } else {
//        FillRoundRectGradientH(x, y, w, h, rx, colFn);
//    }
//}
//
///* ── Outline (unchanged) ─────────────────────────────────────────────────── */
//void DrawRoundRectOutline(uint16_t x,  uint16_t y,
//                          uint16_t w,  uint16_t h,
//                          uint16_t rx, uint16_t col)
//{
//    ILI9488_DrawHLine(x + rx,       y,          w - 2u * rx, col);
//    ILI9488_DrawHLine(x + rx,       y + h - 1u, w - 2u * rx, col);
//    ILI9488_DrawVLine(x,            y + rx,     h - 2u * rx, col);
//    ILI9488_DrawVLine(x + w - 1u,  y + rx,     h - 2u * rx, col);
//
//    ILI9488_DrawPixel(x + rx - 1u, y + rx - 1u,         col);
//    ILI9488_DrawPixel(x + w - rx,  y + rx - 1u,         col);
//    ILI9488_DrawPixel(x + rx - 1u, y + h - rx,          col);
//    ILI9488_DrawPixel(x + w - rx,  y + h - rx,          col);
//}
//
///* ══════════════════════════════════════════════════════════════════════════
//   COLOUR FUNCTIONS
//   ══════════════════════════════════════════════════════════════════════════ */
//uint16_t ColFn_Rainbow(uint16_t i, uint16_t w)
//{
//    static const uint16_t stops[7] = {
//        0xF800u, 0xFD20u, 0xFFE0u,
//        0x07E0u, 0x07FFu, 0x001Fu, 0xF81Fu
//    };
//    uint16_t seg_w = w / 6u;
//    if (seg_w == 0u) seg_w = 1u;
//    uint16_t seg = i / seg_w;
//    if (seg >= 6u) seg = 5u;
//    uint16_t t = (uint16_t)(((uint32_t)(i - seg * seg_w) * 256u) / seg_w);
//    return LerpRGB565(stops[seg], stops[seg + 1u], t);
//}
//
//uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w)
//{
//    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);
//    if (t < 128u)
//        return LerpRGB565(0x0000u, 0x6000u, (uint16_t)(t * 2u));
//    else
//        return LerpRGB565(0x6000u, 0xF800u, (uint16_t)((t - 128u) * 2u));
//}
//
//uint16_t ColFn_AlphaToBlue(uint16_t i, uint16_t w)
//{
//    static const uint16_t CHK_LIGHT = 0xEF7Bu;
//    static const uint16_t CHK_DARK  = 0xCE79u;
//    static const uint16_t BLUE      = 0x001Fu;
//
//    uint16_t cross = (uint16_t)(((uint32_t)w * 40u) / 100u);
//
//    if (i < cross)
//        return ((i / 8u) & 1u) ? CHK_DARK : CHK_LIGHT;
//    else {
//        uint16_t t = (uint16_t)(((uint32_t)(i - cross) * 256u) / (w - cross));
//        return LerpRGB565(CHK_DARK, BLUE, t);
//    }
//}
//
///* ══════════════════════════════════════════════════════════════════════════
//   KNOB DRAWING
//   ══════════════════════════════════════════════════════════════════════════ */
//void DrawSliderKnob(uint16_t x, uint16_t y, uint16_t fill)
//{
//    ILI9488_FillCircle((uint16_t)(x + 2u), (uint16_t)(y + 2u), 12u, 0xBDF7u);
//    ILI9488_FillCircle(x, y, 12u, COL_WHITE);
//    ILI9488_DrawCircle(x, y, 12u, COL_GRAY);
//    ILI9488_FillCircle(x, y, 8u, fill);
//    ILI9488_FillCircle((uint16_t)(x - 3u), (uint16_t)(y - 3u), 3u, COL_WHITE);
//}
//
///* ══════════════════════════════════════════════════════════════════════════
//   MAIN SCENE DRAW
//   ══════════════════════════════════════════════════════════════════════════ */
//void DrawStaticSliderImage(void)
//{
//    /* 1. Background gradient */
//    for (uint16_t y = 0u; y < SCR_H; y++) {
//        uint16_t t   = (uint16_t)(((uint32_t)y * 256u) / SCR_H);
//        uint16_t col = LerpRGB565(BG_TOP, BG_BOT, t);
//        ILI9488_DrawHLine(0u, y, SCR_W, col);
//    }
//
//    /* 2. Three slider tracks
//       SLIDER_n_VERTICAL macros at the top of this file control orientation. */
//    static const struct {
//        uint16_t  cy;
//        uint16_t (*fn)(uint16_t, uint16_t);
//        uint16_t  knob_x;
//        uint16_t  knob_fill;
//        uint8_t   vertical;    /* resolved from compile-time macros below     */
//    } rows[3] = {
//        { ROW_Y1, ColFn_Rainbow,     (uint16_t)(TRACK_X + TRACK_W - 30u), 0xF81Fu, SLIDER1_VERTICAL },
//        { ROW_Y2, ColFn_BlackToRed,  (uint16_t)(TRACK_X + TRACK_W - 10u), 0xF800u, SLIDER2_VERTICAL },
//        { ROW_Y3, ColFn_AlphaToBlue, (uint16_t)(TRACK_X + TRACK_W / 2u),  0x001Fu, SLIDER3_VERTICAL },
//    };
//
//    for (uint8_t r = 0u; r < 3u; r++) {
//        uint16_t ty = (uint16_t)(rows[r].cy - TRACK_H / 2u);
//
//        FillRoundRectGradient(TRACK_X, ty, TRACK_W, TRACK_H,
//                              TRACK_RX, rows[r].fn, rows[r].vertical);
//
//        DrawRoundRectOutline(TRACK_X, ty, TRACK_W, TRACK_H,
//                             TRACK_RX, COL_WHITE);
//
//        /* DrawSliderKnob(rows[r].knob_x, rows[r].cy, rows[r].knob_fill); */
//    }
//}








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
   All per-slider knobs live here. Touch nothing else.
   ══════════════════════════════════════════════════════════════════════════

   ORIENTATION   : 0 = horizontal (left→right),  1 = vertical (top→bottom)
   POSITION      : top-left corner of each track in screen pixels
   SIZE          : width × height of each track
   CORNER RADIUS : pill roundness (should be <= half of the shorter dimension)

   SATURATION_HUE: 0–360  sets the hue of the background gradient.
                   0   = red tint background
                   120 = green tint background
                   240 = blue tint background
                   Any value in between gives the blended hue.
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider 1 : Rainbow / Hue ──────────────────────────────────────────── */
#define S1_VERTICAL    0          /* orientation                             */
#define S1_X           120u       /* left edge  (pixels from left of screen) */
#define S1_Y           14u        /* top edge   (pixels from top of screen)  */
#define S1_W           370u       /* track width                             */
#define S1_H           32u        /* track height                            */
#define S1_RX          16u        /* corner radius                           */
#define S1_KNOB_X      (S1_X + S1_W - 30u)
#define S1_KNOB_FILL   0xF81Fu    /* magenta */

/* ── Slider 2 : Black → Red ────────────────────────────────────────────── */
#define S2_VERTICAL    0
#define S2_X           120u
#define S2_Y           138u
#define S2_W           370u
#define S2_H           32u
#define S2_RX          16u
#define S2_KNOB_X      (S2_X + S2_W - 10u)
#define S2_KNOB_FILL   0xF800u    /* red */

/* ── Slider 3 : Alpha → Blue ───────────────────────────────────────────── */
#define S3_VERTICAL    1
#define S3_X           0u
#define S3_Y           0u
#define S3_W           32u
#define S3_H           220u
#define S3_RX          16u
#define S3_KNOB_X      (S3_X + S3_W / 2u)
#define S3_KNOB_FILL   0x001Fu    /* blue */

/* ── Background saturation hue  (0–360) ────────────────────────────────── */
/*    This hue is blended into the dark top/bottom gradient colours.        */
/*    Set to 0 to get a neutral dark charcoal (original look).              */
#define SATURATION_HUE   0x0000      /* 220 = cool blue-slate tint             */
/* Global or static variable to hold the slider's target hue. 240 is Blue. */
uint16_t g_Slider2_Hue = 240u;
/* ══════════════════════════════════════════════════════════════════════════
   END OF USER-CONFIGURABLE SECTION
   ══════════════════════════════════════════════════════════════════════════ */


/* ══════════════════════════════════════════════════════════════════════════
   HUE → RGB565  (used to tint the background from SATURATION_HUE)
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Convert a hue (0–360) + fixed low saturation + value into RGB565.
 *         S and V are fixed internally so the result is always a dark,
 *         subtle tint suitable for a background.
 * @param  hue   0–360
 * @param  sat   0–255  saturation strength  (suggest 60–120 for backgrounds)
 * @param  val   0–255  brightness           (suggest 30–60  for backgrounds)
 */
static uint16_t HueSVtoRGB565(uint16_t hue, uint8_t sat, uint8_t val)
{
    /* Scale hue into 0–1529 (6 * 255) for integer HSV */
    uint32_t h  = ((uint32_t)hue * 1530u) / 360u;
    uint32_t s  = sat;
    uint32_t v  = val;

    uint32_t r, g, b;
    uint32_t sector = h / 255u;
    uint32_t frac   = h % 255u;

    uint32_t p = (v * (255u - s)) / 255u;
    uint32_t q = (v * (255u - (s * frac) / 255u)) / 255u;
    uint32_t t_v = (v * (255u - (s * (255u - frac)) / 255u)) / 255u;

    switch (sector % 6u) {
        case 0u: r = v;   g = t_v; b = p;   break;
        case 1u: r = q;   g = v;   b = p;   break;
        case 2u: r = p;   g = v;   b = t_v; break;
        case 3u: r = p;   g = q;   b = v;   break;
        case 4u: r = t_v; g = p;   b = v;   break;
        default: r = v;   g = p;   b = q;   break;
    }

    /* Pack to RGB565 */
    return (uint16_t)(((r >> 3u) << 11u) | ((g >> 2u) << 5u) | (b >> 3u));
}

/**
 * @brief  Build top and bottom background colours from SATURATION_HUE.
 *         Returns them via out-parameters so DrawStaticSliderImage can use them.
 */
static void BuildBgColours(uint16_t *bg_top, uint16_t *bg_bot)
{
    /* Top: very dark, low saturation version of the chosen hue */
    *bg_top = HueSVtoRGB565(SATURATION_HUE, 80u,  38u);
    /* Bottom: slightly brighter / more saturated */
    *bg_bot = HueSVtoRGB565(SATURATION_HUE, 100u, 58u);
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
static uint16_t RoundRectClip(uint16_t i, uint16_t w,
                               uint16_t h, uint16_t rx)
{
    (void)h;
    uint16_t clip = 0u;
    if (i < rx) {
        uint16_t dx = rx - i;
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx*dx + (uint32_t)dy*dy <= (uint32_t)rx*rx) {
                clip = rx - dy; break;
            }
        }
    } else if (i >= w - rx) {
        uint16_t dx = i - (w - rx - 1u);
        for (uint16_t dy = 0u; dy < rx; dy++) {
            if ((uint32_t)dx*dx + (uint32_t)dy*dy <= (uint32_t)rx*rx) {
                clip = rx - dy; break;
            }
        }
    }
    return clip;
}

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
                          (uint16_t)(h - 2u * clip), col);
    }
}

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
                          (uint16_t)(w - 2u * hclip), col);
    }
}

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

static void DrawRoundRectOutline(uint16_t x,  uint16_t y,
                                 uint16_t w,  uint16_t h,
                                 uint16_t rx, uint16_t col)
{
    ILI9488_DrawHLine(x + rx,      y,          w - 2u*rx, col);
    ILI9488_DrawHLine(x + rx,      y + h - 1u, w - 2u*rx, col);
    ILI9488_DrawVLine(x,           y + rx,     h - 2u*rx, col);
    ILI9488_DrawVLine(x + w - 1u, y + rx,     h - 2u*rx, col);

    ILI9488_DrawPixel(x + rx - 1u, y + rx - 1u,  col);
    ILI9488_DrawPixel(x + w - rx,  y + rx - 1u,  col);
    ILI9488_DrawPixel(x + rx - 1u, y + h - rx,   col);
    ILI9488_DrawPixel(x + w - rx,  y + h - rx,   col);
}

/* ══════════════════════════════════════════════════════════════════════════
   COLOUR FUNCTIONS
   ══════════════════════════════════════════════════════════════════════════ */
static uint16_t ColFn_Rainbow(uint16_t i, uint16_t w)
{
    static const uint16_t stops[7] = {
        0xF800u, 0xFD20u, 0xFFE0u,
        0x07E0u, 0x07FFu, 0x001Fu, 0xF81Fu
    };
    uint16_t seg_w = w / 6u;
    if (seg_w == 0u) seg_w = 1u;
    uint16_t seg = i / seg_w;
    if (seg >= 6u) seg = 5u;
    uint16_t t = (uint16_t)(((uint32_t)(i - seg * seg_w) * 256u) / seg_w);
    return LerpRGB565(stops[seg], stops[seg + 1u], t);
}

//static uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w)
//{
//    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);
//    if (t < 128u)
//        return LerpRGB565(0x0000u, 0x6000u, (uint16_t)(t * 2u));
//    return LerpRGB565(0x6000u, 0xF800u, (uint16_t)((t - 128u) * 2u));
//}


//static uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w, uint16_t input_colorUg)
//{
//    /* 1. Map the input hue to a dark and bright RGB565 color
//          using your existing HueSVtoRGB565 helper function. */
//    uint16_t dark_col   = HueSVtoRGB565(input_colorUg, 255u, 100u); /* Replaces 0x6000u (dark red)   */
//    uint16_t bright_col = HueSVtoRGB565(input_colorUg, 255u, 255u); /* Replaces 0xF800u (bright red) */
//
//    /* 2. Original 2-stage interpolation logic */
//    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);
//
//    if (t < 128u) {
//        /* Black -> Dark Color */
//        return LerpRGB565(0x0000u, dark_col, (uint16_t)(t * 2u));
//    } else {
//        /* Dark Color -> Bright Color */
//        return LerpRGB565(dark_col, bright_col, (uint16_t)((t - 128u) * 2u));
//    }
//}

static uint16_t ColFn_BlackToRed(uint16_t i, uint16_t w)
{
    /* Map the global hue to dark and bright colors */
    uint16_t dark_col   = HueSVtoRGB565(g_Slider2_Hue, 255u, 100u);
    uint16_t bright_col = HueSVtoRGB565(g_Slider2_Hue, 255u, 255u);

    /* 2-stage interpolation logic */
    uint16_t t = (uint16_t)(((uint32_t)i * 256u) / w);

    if (t < 128u) {
        /* Black -> Dark Color */
        return LerpRGB565(0x0000u, dark_col, (uint16_t)(t * 2u));
    } else {
        /* Dark Color -> Bright Color */
        return LerpRGB565(dark_col, bright_col, (uint16_t)((t - 128u) * 2u));
    }
}


static uint16_t ColFn_AlphaToBlue(uint16_t i, uint16_t w)
{
    static const uint16_t CHK_LIGHT = 0xEF7Bu;
    static const uint16_t CHK_DARK  = 0xCE79u;
    static const uint16_t BLUE      = 0x001Fu;
    uint16_t cross = (uint16_t)(((uint32_t)w * 40u) / 100u);
    if (i < cross)
        return ((i / 8u) & 1u) ? CHK_DARK : CHK_LIGHT;
    uint16_t t = (uint16_t)(((uint32_t)(i - cross) * 256u) / (w - cross));
    return LerpRGB565(CHK_DARK, BLUE, t);
}

/* ══════════════════════════════════════════════════════════════════════════
   KNOB DRAWING
   ══════════════════════════════════════════════════════════════════════════ */
static void DrawSliderKnob(uint16_t x, uint16_t y, uint16_t fill)
{
    ILI9488_FillCircle((uint16_t)(x+2u),(uint16_t)(y+2u),12u,0xBDF7u);
    ILI9488_FillCircle(x, y, 12u, COL_WHITE);
    ILI9488_DrawCircle(x, y, 12u, COL_GRAY);
    ILI9488_FillCircle(x, y,  8u, fill);
    ILI9488_FillCircle((uint16_t)(x-3u),(uint16_t)(y-3u),3u,COL_WHITE);
}

/* ══════════════════════════════════════════════════════════════════════════
   MAIN SCENE DRAW
   ══════════════════════════════════════════════════════════════════════════ */
void DrawStaticSliderImage(void)
{
    /* ── 1. Background — hue derived from SATURATION_HUE ─────────────── */
    uint16_t bg_top, bg_bot;
    BuildBgColours(&bg_top, &bg_bot);

    for (uint16_t y = 0u; y < SCR_H; y++) {
        uint16_t t   = (uint16_t)(((uint32_t)y * 256u) / SCR_H);
        uint16_t col = LerpRGB565(bg_top, bg_bot, t);
        ILI9488_DrawHLine(0u, y, SCR_W, col);
    }

    /* ── 2. Three slider tracks ──────────────────────────────────────── */
    static const struct {
        uint16_t  x, y, w, h, rx;                   /* geometry           */
        uint16_t  knob_x, knob_cy, knob_fill;        /* knob               */
        uint8_t   vertical;                           /* orientation        */
        uint16_t (*fn)(uint16_t, uint16_t);           /* colour function    */
    } sliders[3] = {
        /* Slider 1 — Rainbow */
        {
            S1_X, S1_Y, S1_W, S1_H, S1_RX,
            S1_KNOB_X, (uint16_t)(S1_Y + S1_H/2u), S1_KNOB_FILL,
            S1_VERTICAL, ColFn_Rainbow
        },
        /* Slider 2 — Black→Red */
        {
            S2_X, S2_Y, S2_W, S2_H, S2_RX,
            S2_KNOB_X, (uint16_t)(S2_Y + S2_H/2u), S2_KNOB_FILL,
            S2_VERTICAL, ColFn_BlackToRed
        },
        /* Slider 3 — Alpha→Blue */
        {
            S3_X, S3_Y, S3_W, S3_H, S3_RX,
            S3_KNOB_X, (uint16_t)(S3_Y + S3_H/2u), S3_KNOB_FILL,
            S3_VERTICAL, ColFn_AlphaToBlue
        },
    };

    for (uint8_t r = 0u; r < 3u; r++) {
        FillRoundRectGradient(sliders[r].x, sliders[r].y,
                              sliders[r].w, sliders[r].h,
                              sliders[r].rx,
                              sliders[r].fn,
                              sliders[r].vertical);

        DrawRoundRectOutline(sliders[r].x, sliders[r].y,
                             sliders[r].w, sliders[r].h,
                             sliders[r].rx, COL_WHITE);

        /* DrawSliderKnob(sliders[r].knob_x,
                          sliders[r].knob_cy,
                          sliders[r].knob_fill); */
    }
}
