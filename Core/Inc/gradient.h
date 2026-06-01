/**
 ******************************************************************************
 * @file    gradient.h
 * @brief   Gradient slider background drawing API
 ******************************************************************************
 */

#ifndef GRADIENT_H
#define GRADIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════════
   GLOBAL STATE
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Current hue (0–360) used by ColFn_BlackToRed (slider 2 track).
 *         Write this then call GRADIENT_RedrawSlider2Track().
 */
extern uint16_t g_Slider2_Hue;

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC API
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Draw the complete static scene:
 *           1. Full-screen dark gradient background
 *           2. All three slider track pills + outlines
 *         Call once after UGFX_Commit() on startup.
 */
void DrawStaticSliderImage(void);

/**
 * @brief  Repaint ONLY the slider-2 pill (Black → hue colour).
 *         No background repaint, no other sliders touched.
 *         Call after updating g_Slider2_Hue for a zero-flicker live update.
 */
void GRADIENT_RedrawSlider2Track(void);

#ifdef __cplusplus
}
#endif

#endif /* GRADIENT_H */