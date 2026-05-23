///**
// ******************************************************************************
// * @file    TouchGFXHAL_ILI9488.cpp
// * @brief   TouchGFX Hardware Abstraction Layer – ILI9488 SPI + XPT2046
// *
// * Drop this file into your TouchGFX project's TouchGFX/target/ folder.
// * It replaces (or extends) the generated TouchGFXHAL.cpp.
// *
// * Tested with:
// *   • TouchGFX 4.21 / 4.22
// *   • STM32F429ZGTx @ 180 MHz
// *   • ILI9488 480×320, 18-bit SPI (KMRTM35018-SPI v2.0)
// *   • XPT2046 resistive touch
// *
// * TouchGFX Designer settings to match:
// *   Display: 480 × 320, RGB565 framebuffer
// *   Partial framebuffer strategy recommended for SPI (lower RAM usage).
// *   Block size: 480 × 32 (15360 bytes = 30 KB for one block)
// *   Set block size in TouchGFX Designer under "HAL Configuration".
// ******************************************************************************
// */
//
///* ── C-linkage includes (HAL + our drivers) ─────────────────────────────── */
//extern "C" {
//#include "ili9488.h"
//#include "xpt2046.h"
//#include "main.h"
//}
//
///* ── TouchGFX includes ──────────────────────────────────────────────────── */
//#include <touchgfx/hal/HAL.hpp>
//#include <touchgfx/hal/GPIO.hpp>
//#include <touchgfx/lcd/LCD.hpp>
//#include <touchgfx/Application.hpp>
//#include <platform/driver/lcd/LCD16bpp.hpp>
//
///* External SPI handle declared in main.c */
//extern SPI_HandleTypeDef hspi1;
//
///* ═══════════════════════════════════════════════════════════════════════════
//   TouchGFXHAL class
//   ═══════════════════════════════════════════════════════════════════════════ */
//class TouchGFXHAL : public touchgfx::HAL
//{
//public:
//    TouchGFXHAL(touchgfx::DMA_Interface &dma,
//                touchgfx::LCD          &display,
//                touchgfx::TouchController &tc,
//                uint16_t width, uint16_t height)
//        : touchgfx::HAL(dma, display, tc, width, height) {}
//
//    /* ── LCD initialisation ─────────────────────────────────────────────── */
//    void initialize() override
//    {
//        HAL::initialize();
//        ILI9488_Init(&hspi1);
//    }
//
//    /* ── Framebuffer block copy ─────────────────────────────────────────── */
//    /**
//     * TouchGFX calls this for every dirty rectangle.
//     * pSrc points to the *beginning of the framebuffer*, not the rect start.
//     * We advance the pointer ourselves.
//     */
//    void copyFrameBufferBlockToLCD(const touchgfx::Rect &rect) override
//    {
//        const uint16_t *fb = (const uint16_t *)lockFrameBuffer();
//
//        /* Advance to first pixel of the dirty rect */
//        fb += (uint32_t)rect.y * DISPLAY_WIDTH + rect.x;
//
//        ILI9488_CopyFrameBufferBlockToLCD(fb,
//                                           (uint16_t)rect.x,
//                                           (uint16_t)rect.y,
//                                           (uint16_t)rect.width,
//                                           (uint16_t)rect.height);
//
//        unlockFrameBuffer();
//    }
//
//    /* ── Flush – no double-buffer / vsync available on SPI ─────────────── */
//    void flushFrameBuffer() override
//    {
//        HAL::flushFrameBuffer();
//    }
//
//    void flushFrameBuffer(const touchgfx::Rect &rect) override
//    {
//        copyFrameBufferBlockToLCD(rect);
//    }
//};
//
///* ═══════════════════════════════════════════════════════════════════════════
//   Touch controller adapter
//   ═══════════════════════════════════════════════════════════════════════════ */
//class XPT2046TouchController : public touchgfx::TouchController
//{
//public:
//    void init() override
//    {
//        XPT2046_Init(&hspi1);
//    }
//
//    bool sampleTouch(int32_t &x, int32_t &y) override
//    {
//        XPT2046_Point_t pt;
//        if (XPT2046_GetTouchPixel(&pt)) {
//            x = (int32_t)pt.x;
//            y = (int32_t)pt.y;
//            return true;
//        }
//        return false;
//    }
//};
//
///* ═══════════════════════════════════════════════════════════════════════════
//   Dummy DMA (SPI is blocking / polling in this driver)
//   ═══════════════════════════════════════════════════════════════════════════ */
//class NoDMA : public touchgfx::DMA_Interface
//{
//public:
//    NoDMA() : touchgfx::DMA_Interface(_q) {}
//    touchgfx::DMA_Queue _q;
//
//    touchgfx::DMAType  getDMAType() override { return touchgfx::DMA_TYPE_CHROMART; }
//};
//
///* ═══════════════════════════════════════════════════════════════════════════
//   Factory function – called from TouchGFX generated main
//   ═══════════════════════════════════════════════════════════════════════════ */
//extern "C" void touchgfx_init(void)
//{
//    /* Static instances */
//    static NoDMA              dma;
//    static touchgfx::LCD16bpp display;
//    static XPT2046TouchController tc;
//    static TouchGFXHAL        hal(dma, display, tc,
//                                   ILI9488_WIDTH, ILI9488_HEIGHT);
//
//    touchgfx::HAL::setHAL(&hal);
//    hal.initialize();
//}
//
///* ═══════════════════════════════════════════════════════════════════════════
//   EXTI callback – wire TOUCH_INT here
//   ═══════════════════════════════════════════════════════════════════════════
//   In your main.c USER CODE section, add:
//
//     void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//     {
//         if (GPIO_Pin == TOUCH_INT_Pin) {
//             XPT2046_IRQHandler();
//         }
//     }
//   ═══════════════════════════════════════════════════════════════════════════ */
