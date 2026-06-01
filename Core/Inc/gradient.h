#ifndef GRADIENT_H
#define GRADIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
extern uint16_t g_Slider2_Hue;

/**
 * @brief Draw complete static slider background image
 *        (gradient background + slider tracks).
 */
void DrawStaticSliderImage(void);

#ifdef __cplusplus
}
#endif

#endif /* GRADIENT_H */
