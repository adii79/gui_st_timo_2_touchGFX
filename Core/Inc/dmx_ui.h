/**
 ******************************************************************************
 * @file    dmx_ui.h
 * @brief   DMX512 UI layer — screen state machine + channel mapping
 *
 * SCREEN / MEMORY STRATEGY
 * ──────────────────────────────────────────────────────────────────────────
 *  Like the Arduino u8g reference code, we keep inactive screens as compact
 *  descriptors in flash (PROGMEM equivalent = const structs).  Only the
 *  *active* screen's widgets are built into µGFX's static RAM pools via
 *  UGFX_Begin() … UGFX_Commit().  Switching screens tears down the old
 *  widgets (UGFX_Begin() resets the pools) and builds only the new set.
 *
 *  Screens:
 *    SCREEN_MAIN   — home: 3 RGB sliders + channel readout label
 *    SCREEN_GROUP  — 6 scene-preset buttons (calls DMX scene snapshots)
 *    SCREEN_DETAIL — per-channel fine-tune: slider + up/down nudge buttons
 *
 * DMX layout (512 channels):
 *    Ch 1..3    = RGB fixture 1  (sliders on SCREEN_MAIN)
 *    Ch 4..6    = RGB fixture 2
 *    …          = extend as needed
 *    Ch 500..505 = scene preset triggers (buttons on SCREEN_GROUP)
 *    Ch 506..512 = spare
 ******************************************************************************
 */
#ifndef DMX_UI_H
#define DMX_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ugfx.h"
#include <stdint.h>
#include <stdbool.h>

/* ── DMX universe ────────────────────────────────────────────────────────── */
#define DMX_CHANNELS        512u
#define DMX_CH_RED          1u    /* 1-indexed, like a real DMX console     */
#define DMX_CH_GREEN        2u
#define DMX_CH_BLUE         3u
#define DMX_CH_MASTER_DIM   4u
#define DMX_CH_STROBE       5u
#define DMX_CH_MODE         6u

/* Scene preset channels (one byte per scene: 0=off, 255=trigger) */
#define DMX_CH_SCENE_BASE   500u
#define DMX_NUM_SCENES      6u

/* ── Screen IDs ──────────────────────────────────────────────────────────── */
typedef enum {
    SCREEN_MAIN   = 0,  /* RGB sliders                                      */
    SCREEN_GROUP  = 1,  /* 6 scene buttons                                  */
    SCREEN_DETAIL = 2,  /* fine-tune one channel                            */
    SCREEN_COUNT
} dmx_screen_t;

/* ── Flash screen descriptor (kept in .rodata / flash, zero RAM cost) ────── */
typedef struct {
    const char   *title;        /* shown in status bar                      */
    void        (*build_fn)(void);  /* called when screen becomes active    */
} dmx_screen_desc_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/**
 * @brief  Call once after UGFX_Init().  Builds the initial screen
 *         (SCREEN_MAIN) into RAM and draws it.
 */
void DMX_UI_Init(void);

/**
 * @brief  Call every loop iteration after UGFX_Poll().
 *         Handles dirty labels, deferred screen transitions, and
 *         schedules the DMX break+data frame via your UART/DMA driver.
 */
void DMX_UI_Process(void);

/**
 * @brief  Transition to a new screen.  Old widgets are destroyed (pool
 *         reset), new screen's build_fn() allocates fresh widgets from
 *         the static pool, then UGFX_Commit() redraws.
 */
void DMX_UI_GotoScreen(dmx_screen_t screen);

/**
 * @brief  Read a DMX channel value (0-255). ch is 1-indexed.
 */
uint8_t  DMX_GetChannel(uint16_t ch);

/**
 * @brief  Write a DMX channel value (0-255). ch is 1-indexed.
 *         Also marks the universe dirty so the next DMX frame sends it.
 */
void     DMX_SetChannel(uint16_t ch, uint8_t value);

/**
 * @brief  Set a named scene snapshot: fills ch 1..6 with preset values.
 *         scene index 0..5.
 */
void     DMX_LoadScene(uint8_t scene_idx);

#ifdef __cplusplus
}
#endif
#endif /* DMX_UI_H */
