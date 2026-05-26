/**
 ******************************************************************************
 * @file    ui_test.c
 * @brief   Test screen: icon row · button · two sliders · bouncing ball
 *
 * Screen layout (480 × 320 landscape)
 * ┌────────────────────────────────────────────────────────────┐
 * │  [🏠]  [🔔]  [⚙]  [🎵]  [📶]  [🔋]   ← icon strip       │
 * │  ─────────────────────────────────────────────────────     │
 * │                                                            │
 * │  ┌──────────────┐           ● bouncing ball               │
 * │  │  PRESS ME    │           in a bounded zone             │
 * │  └──────────────┘                                         │
 * │                                                            │
 * │  VOL  ──────[●]────  73%    sliders with                  │
 * │  BRT  ─[●]─────────  24%    floating value label          │
 * │                                                            │
 * │  status bar                                                │
 * └────────────────────────────────────────────────────────────┘
 ******************************************************************************
 */

#include "ui_test.h"
#include "ui_gfx.h"
#include "ili9488.h"
#include "xpt2046.h"
#include <stdio.h>
#include <string.h>

/* ─── Layout ────────────────────────────────────────────────────────────── */

#define COL_BG          0x0000u
#define COL_DIVIDER     0x2104u
#define COL_LABEL       0x8C71u   /* medium grey                            */
#define COL_WHITE       0xFFFFu
#define COL_YELLOW      0xFFE0u
#define COL_CYAN        0x07FFu
#define COL_GREEN       0x07E0u
#define COL_RED         0xF800u
#define COL_ORANGE      0xFD20u

/* Icon strip */
#define ICON_STRIP_Y    14u
#define ICON_STRIP_X0   14u
#define ICON_STEP       58u    /* horizontal spacing between icons          */
#define ICON_SCALE       2u    /* all icons drawn at 2×                     */

/* Divider */
#define DIVIDER_Y       54u

/* Button */
#define BTN_X           210u
#define BTN_Y           160u
#define BTN_W          150u
#define BTN_H           46u

/* Sliders */
#define SLD1_X          50u
#define SLD1_Y         240u
#define SLD1_W         100u

#define SLD2_X          50u
#define SLD2_Y        220u
#define SLD2_W         100u

/* Ball zone (right half) */
#define BALL_ZONE_X    360u
#define BALL_ZONE_Y     4u
#define BALL_ZONE_W    122u
#define BALL_ZONE_H    230u
#define BALL_R          10u
#define BALL_STEP_X      3
#define BALL_STEP_Y      2
#define BALL_TICK_MS    12u

/* Status bar */
#define STATUS_X        14u
#define STATUS_Y       298u
#define STATUS_W       460u
#define STATUS_H        18u

/* ─── State ─────────────────────────────────────────────────────────────── */

static UI_Slider_t s_vol;
static UI_Slider_t s_brt;

static uint8_t  s_btn_pressed  = 0u;
static uint32_t s_btn_count    = 0u;
static uint8_t  s_was_touched  = 0u;

static int16_t  s_ball_x, s_ball_y;
static int16_t  s_ball_dx = BALL_STEP_X, s_ball_dy = BALL_STEP_Y;
static int16_t  s_ball_prev_x = -1, s_ball_prev_y = -1;
static uint32_t s_ball_last_tick = 0u;

/* ─── Icon descriptors ──────────────────────────────────────────────────── */

static const UI_Bitmap_t bmp_d_home     = BMP_DESC_HOME;
static const UI_Bitmap_t bmp_d_bell     = BMP_DESC_BELL;
static const UI_Bitmap_t bmp_d_settings = BMP_DESC_SETTINGS;
static const UI_Bitmap_t bmp_d_music    = BMP_DESC_MUSIC;
static const UI_Bitmap_t bmp_d_wifi     = BMP_DESC_WIFI;
static const UI_Bitmap_t bmp_d_battery  = BMP_DESC_BATTERY;

/* ─── Button helpers ────────────────────────────────────────────────────── */

static void _DrawButton(uint8_t pressed)
{
    uint16_t fill   = pressed ? 0x4228u : 0x2104u;
    uint16_t border = pressed ? COL_CYAN : COL_WHITE;

    ILI9488_FillRect(BTN_X, BTN_Y, BTN_W, BTN_H, fill);
    ILI9488_DrawRect(BTN_X, BTN_Y, BTN_W, BTN_H, border);

    /* inner second border for depth */
    if (!pressed)
        ILI9488_DrawRect((uint16_t)(BTN_X + 1u), (uint16_t)(BTN_Y + 1u),
                         (uint16_t)(BTN_W - 2u), (uint16_t)(BTN_H - 2u),
                         0x39E7u);

    ILI9488_DrawString((uint16_t)(BTN_X + 20u), (uint16_t)(BTN_Y + 15u),
                       "PRESS ME", COL_WHITE, fill, 2u);
}

static uint8_t _InButton(uint16_t x, uint16_t y)
{
    return (x >= BTN_X && x < BTN_X + BTN_W &&
            y >= BTN_Y && y < BTN_Y + BTN_H) ? 1u : 0u;
}

/* ─── Status bar ─────────────────────────────────────────────────────────── */

static void _Status(const char *msg)
{
    ILI9488_FillRect(STATUS_X, STATUS_Y, STATUS_W, STATUS_H, COL_BG);
    ILI9488_DrawString(STATUS_X, STATUS_Y, msg, COL_GREEN, COL_BG, 1u);
}

/* ─── Ball animation ────────────────────────────────────────────────────── */

static void _StepBall(void)
{
    /* erase old */
    if (s_ball_prev_x >= 0) {
        uint16_t ex = (uint16_t)(s_ball_prev_x - BALL_R - 1);
        uint16_t ey = (uint16_t)(s_ball_prev_y - BALL_R - 1);
        uint16_t es = (uint16_t)(BALL_R * 2u + 2u);
        ILI9488_FillRect(ex, ey, es, es, COL_BG);
    }

    s_ball_x += s_ball_dx;
    s_ball_y += s_ball_dy;

    if (s_ball_x - BALL_R < (int16_t)BALL_ZONE_X)
        { s_ball_x = (int16_t)(BALL_ZONE_X + BALL_R); s_ball_dx =  BALL_STEP_X; }
    if (s_ball_x + BALL_R > (int16_t)(BALL_ZONE_X + BALL_ZONE_W))
        { s_ball_x = (int16_t)(BALL_ZONE_X + BALL_ZONE_W - BALL_R); s_ball_dx = -BALL_STEP_X; }
    if (s_ball_y - BALL_R < (int16_t)BALL_ZONE_Y)
        { s_ball_y = (int16_t)(BALL_ZONE_Y + BALL_R); s_ball_dy =  BALL_STEP_Y; }
    if (s_ball_y + BALL_R > (int16_t)(BALL_ZONE_Y + BALL_ZONE_H))
        { s_ball_y = (int16_t)(BALL_ZONE_Y + BALL_ZONE_H - BALL_R); s_ball_dy = -BALL_STEP_Y; }

    /* draw with highlight */
    ILI9488_FillCircle((uint16_t)s_ball_x, (uint16_t)s_ball_y, BALL_R, COL_YELLOW);
    ILI9488_FillCircle((uint16_t)(s_ball_x - 3u), (uint16_t)(s_ball_y - 3u),
                       3u, COL_WHITE);  /* specular dot */

    s_ball_prev_x = s_ball_x;
    s_ball_prev_y = s_ball_y;
}

/* ═══════════════════════════════════════════════════════════════════════════
   Public API
   ═══════════════════════════════════════════════════════════════════════════ */

void UI_Test_Init(void)
{
    ILI9488_FillScreen(COL_BG);

    /* ── Icon strip ──────────────────────────────────────────────────────── */
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 0u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_home,     ICON_SCALE, COL_CYAN);
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 1u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_bell,     ICON_SCALE, COL_YELLOW);
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 2u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_settings, ICON_SCALE, COL_WHITE);
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 3u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_music,    ICON_SCALE, COL_ORANGE);
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 4u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_wifi,     ICON_SCALE, COL_GREEN);
    UI_DrawBitmap((uint16_t)(ICON_STRIP_X0 + 5u * ICON_STEP), ICON_STRIP_Y,
                  &bmp_d_battery,  ICON_SCALE, COL_GREEN);

    /* ── Divider ─────────────────────────────────────────────────────────── */
    ILI9488_DrawHLine(0u, DIVIDER_Y, ILI9488_WIDTH, COL_DIVIDER);

    /* ── Button ──────────────────────────────────────────────────────────── */
    _DrawButton(0u);

    /* ── Slider labels ───────────────────────────────────────────────────── */
    ILI9488_DrawString(14u, (uint16_t)(SLD1_Y - 12u),
                       "VOL", COL_LABEL, COL_BG, 1u);
    ILI9488_DrawString(14u, (uint16_t)(SLD2_Y - 12u),
                       "BRT", COL_LABEL, COL_BG, 1u);

    /* ── Sliders ─────────────────────────────────────────────────────────── */
    UI_Slider_Init(&s_vol, SLD1_X, SLD1_Y, SLD1_W, 0, 100, 60);
    UI_Slider_Draw(&s_vol);

    UI_Slider_Init(&s_brt, SLD2_X, SLD2_Y, SLD2_W, 0, 100, 30);
    UI_Slider_Draw(&s_brt);

    /* ── Ball zone border ────────────────────────────────────────────────── */
    ILI9488_DrawRect((uint16_t)(BALL_ZONE_X - 1u), (uint16_t)(BALL_ZONE_Y - 1u),
                     (uint16_t)(BALL_ZONE_W + 2u), (uint16_t)(BALL_ZONE_H + 2u),
                     COL_DIVIDER);

    /* ── Ball init ───────────────────────────────────────────────────────── */
    s_ball_x = (int16_t)(BALL_ZONE_X + BALL_ZONE_W / 2);
    s_ball_y = (int16_t)(BALL_ZONE_Y + BALL_ZONE_H / 2);

    /* ── Status ──────────────────────────────────────────────────────────── */
    ILI9488_DrawHLine(0u, (uint16_t)(STATUS_Y - 4u), ILI9488_WIDTH, COL_DIVIDER);
    _Status("Ready — tap button, drag sliders");
}

void UI_Test_Poll(void)
{
    /* ── Touch ───────────────────────────────────────────────────────────── */
    XPT2046_Point_t pt;
    uint8_t touching = XPT2046_GetTouchPixel(&pt) ? 1u : 0u;

    if (touching) {
        /* Button */
        if (_InButton(pt.x, pt.y)) {
            if (!s_btn_pressed) {
                s_btn_pressed = 1u;
                _DrawButton(1u);
            }
        } else if (s_btn_pressed) {
            /* slid off — cancel */
            s_btn_pressed = 0u;
            _DrawButton(0u);
        }

        /* Sliders — feed both; only the one being dragged will respond */
        if (UI_Slider_Touch(&s_vol, pt.x, pt.y)) {
            char buf[32];
            snprintf(buf, sizeof(buf),
                     "Volume: %d%%   Brightness: %d%%   ",
                     (int)UI_Slider_GetValue(&s_vol),
                     (int)UI_Slider_GetValue(&s_brt));
            _Status(buf);
        }
        if (UI_Slider_Touch(&s_brt, pt.x, pt.y)) {
            char buf[32];
            snprintf(buf, sizeof(buf),
                     "Volume: %d%%   Brightness: %d%%   ",
                     (int)UI_Slider_GetValue(&s_vol),
                     (int)UI_Slider_GetValue(&s_brt));
            _Status(buf);
        }

    } else {
        /* Touch UP */
        if (s_was_touched) {
            /* confirm button tap on release */
            if (s_btn_pressed) {
                s_btn_pressed = 0u;
                s_btn_count++;
                _DrawButton(0u);

                char buf[48];
                snprintf(buf, sizeof(buf),
                         "Button #%lu!  Vol=%d%%  Brt=%d%%   ",
                         (unsigned long)s_btn_count,
                         (int)UI_Slider_GetValue(&s_vol),
                         (int)UI_Slider_GetValue(&s_brt));
                _Status(buf);
            }
            /* release both sliders */
            UI_Slider_Release(&s_vol);
            UI_Slider_Release(&s_brt);
        }
    }

    s_was_touched = touching;

    /* ── Ball animation ──────────────────────────────────────────────────── */
    uint32_t now = HAL_GetTick();
    if ((now - s_ball_last_tick) >= BALL_TICK_MS) {
        s_ball_last_tick = now;
        _StepBall();
    }
}
