/**
 ******************************************************************************
 * @file    ui_test.h
 * @brief   GUI proof-of-concept test layer for ILI9488 + XPT2046
 *
 * Tests four things on one screen — no RTOS, no extra libs:
 *   1. 1-bit bitmap icon drawn in any fg/bg colour
 *   2. Touchable button  → prints to screen + optional UART
 *   3. Draggable slider  → dirty-rect redraws knob only
 *   4. Bouncing ball animation → dirty-rect redraws ball bbox only
 *
 * Usage in main.c:
 *   #include "ui_test.h"
 *   // after ILI9488_Init() + XPT2046_Init():
 *   UI_Test_Init();
 *   while (1) { UI_Test_Poll(); }
 ******************************************************************************
 */

#ifndef UI_TEST_H
#define UI_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"  /* pulls in stm32f4xx_hal.h, GPIO defines */

void UI_Test_Init(void);   /* draw initial scene once            */
void UI_Test_Poll(void);   /* call every loop tick — no HAL_Delay inside */

#ifdef __cplusplus
}
#endif
#endif /* UI_TEST_H */
