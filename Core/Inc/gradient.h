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
 *         Call once after ILI9488_FillScreen(), BEFORE UGFX_Commit().
 */
void DrawStaticSliderImage(void);

/**
 * @brief  Repaint ONLY the slider-1 pill (Rainbow hue, vertical).
 *         No background, no other sliders touched.
 */
void GRADIENT_RedrawSlider1Track(void);

/**
 * @brief  Repaint ONLY the slider-2 pill (Black → dynamic hue, horizontal).
 *         Call after updating g_Slider2_Hue for a zero-flicker live update.
 */
void GRADIENT_RedrawSlider2Track(void);

/**
 * @brief  Repaint ONLY the slider-3 pill (Checkerboard → Blue, horizontal).
 *         No background, no other sliders touched.
 */
void GRADIENT_RedrawSlider3Track(void);

#ifdef __cplusplus
}
#endif

#endif /* GRADIENT_H */
