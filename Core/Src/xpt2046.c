/**
 ******************************************************************************
 * @file    xpt2046.c
 * @brief   XPT2046 Resistive Touchscreen Driver
 *          Shares SPI1 bus with ILI9488.  Uses separate TOUCH_CS (PF2).
 *          TOUCH_INT (PF3) is configured as EXTI rising-edge in CubeMX.
 *
 * SPI timing requirements for XPT2046:
 *   • CPOL=0, CPHA=0  (SPI Mode 0) – matches display, so same SPI config OK
 *   • Max DCLK = 2 MHz for accurate ADC readings.
 *   • STM32F429 SPI1 at 180/2 = 90 MHz → use SPI prescaler 64 = 1.4 MHz
 *     when sampling touch; switch back to /2 for display transfers.
 *     (Alternatively use a software SPI or ensure your SPI prescaler is ≤32.)
 *     The driver handles prescaler switching transparently.
 *
 * Calibration:
 *   The raw XPT2046 ADC outputs (0-4095) must be mapped to pixel coords.
 *   A 3-point affine transform is used.  Store the XPT2046_Calib_t in
 *   non-volatile memory (e.g. last sector of flash) and call
 *   XPT2046_SetCalibration() on boot.
 *
 *   Default (uncalibrated) fall-back uses simple linear mapping.
 ******************************************************************************
 */

#include "xpt2046.h"
#include "ili9488.h"   /* For ILI9488_WIDTH / HEIGHT, CS helpers */
#include <string.h>

/* ─── Private state ─────────────────────────────────────────────────────── */
static SPI_HandleTypeDef *_hspi    = NULL;
static volatile bool      _touched = false;
static XPT2046_Calib_t    _calib   = { .valid = false };

/* ─── XPT2046 control byte: channel select + mode ──────────────────────── */
#define XPT2046_CMD_X    0xD0u   /* 12-bit, differential, X  */
#define XPT2046_CMD_Y    0x90u   /* 12-bit, differential, Y  */
#define XPT2046_CMD_Z1   0xB0u   /* pressure measurement     */
#define XPT2046_CMD_Z2   0xC0u

/* Number of samples to average (reduces noise) */
#define XPT2046_AVG_SAMPLES  4u

/* Touch pressure threshold (higher Z1-Z2 ratio = more pressure) */
#define XPT2046_Z_THRESHOLD  200u

/* ─── SPI prescaler switching ───────────────────────────────────────────── */
/* SPI1_CR1 BaudRate field bits [5:3] – prescaler divider value              */
#define SPI_BAUDRATE_DIV2    0x00u
#define SPI_BAUDRATE_DIV64   0x05u

static void _SPI_SetPrescaler(uint8_t prescalerBits)
{
    /* Disable SPI, change prescaler, re-enable */
    _hspi->Instance->CR1 &= ~SPI_CR1_SPE;
    _hspi->Instance->CR1 &= ~SPI_CR1_BR_Msk;
    _hspi->Instance->CR1 |= (uint32_t)prescalerBits << SPI_CR1_BR_Pos;
    _hspi->Instance->CR1 |=  SPI_CR1_SPE;
}

/* ─── Low-level SPI for touch (slow) ───────────────────────────────────── */

static inline void _TOUCH_CS_Assert(void)
{
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET);
}
static inline void _TOUCH_CS_Deassert(void)
{
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief  Read one 12-bit sample from the XPT2046 for the given command.
 *         CS must already be asserted.
 */
static uint16_t _ReadChannel(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };
    HAL_SPI_TransmitReceive(_hspi, tx, rx, 3, HAL_MAX_DELAY);
    /* Result: rx[1] bits[6:0] << 5 | rx[2] bits[7:3] >> 3 → 12 bits */
    return (uint16_t)(((uint16_t)rx[1] << 4u) | ((uint16_t)rx[2] >> 4u));
}

/**
 * @brief  Average N samples for one channel.
 */
static uint16_t _SampleAvg(uint8_t cmd)
{
    uint32_t acc = 0;
    for (uint8_t i = 0; i < XPT2046_AVG_SAMPLES; i++) {
        acc += _ReadChannel(cmd);
    }
    return (uint16_t)(acc / XPT2046_AVG_SAMPLES);
}

/* ─── Public API ─────────────────────────────────────────────────────────── */

void XPT2046_Init(SPI_HandleTypeDef *hspi)
{
    _hspi = hspi;
    _TOUCH_CS_Deassert();
    _touched = false;
    memset(&_calib, 0, sizeof(_calib));
    _calib.valid = false;
}

bool XPT2046_IsTouched(void)
{
    /* Quick check via INT pin state (active LOW = touched) */
    return (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);
}

bool XPT2046_GetTouchRaw(uint16_t *rawX, uint16_t *rawY, uint16_t *rawZ)
{
    /* Switch SPI to slow speed for ADC accuracy */
    _SPI_SetPrescaler(SPI_BAUDRATE_DIV64);

    _TOUCH_CS_Assert();

    /* Measure pressure first – discard if nothing is touching */
    uint16_t z1 = _ReadChannel(XPT2046_CMD_Z1);
    uint16_t z2 = _ReadChannel(XPT2046_CMD_Z2);

    /* Pressure = Z1/4096 - Z2/4096 + 1  (simplified: z1 - z2 + 4096) */
    int32_t pressure = (int32_t)z1 - (int32_t)z2 + 4096;

    if (pressure < (int32_t)XPT2046_Z_THRESHOLD) {
        _TOUCH_CS_Deassert();
        _SPI_SetPrescaler(SPI_BAUDRATE_DIV2);
        return false;
    }

    uint16_t x = _SampleAvg(XPT2046_CMD_X);
    uint16_t y = _SampleAvg(XPT2046_CMD_Y);

    _TOUCH_CS_Deassert();
    _SPI_SetPrescaler(SPI_BAUDRATE_DIV2);

    if (rawX) *rawX = x;
    if (rawY) *rawY = y;
    if (rawZ) *rawZ = (uint16_t)(pressure > 4095 ? 4095 : pressure);

    return true;
}

bool XPT2046_GetTouchPixel(XPT2046_Point_t *pt)
{
    if (!pt) return false;

    uint16_t rawX, rawY, rawZ;
    if (!XPT2046_GetTouchRaw(&rawX, &rawY, &rawZ)) return false;

    pt->z = rawZ;

    if (_calib.valid) {
        /* Affine transform:
         *   pixel_x = (alphaX * rawX + betaX) / divider
         *   pixel_y = (alphaY * rawY + betaY) / divider
         */
        int32_t px = (_calib.alphaX * (int32_t)rawX + _calib.betaX) / _calib.divider;
        int32_t py = (_calib.alphaY * (int32_t)rawY + _calib.betaY) / _calib.divider;

        /* Clamp to display bounds */
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px >= (int32_t)ILI9488_WIDTH)  px = ILI9488_WIDTH  - 1;
        if (py >= (int32_t)ILI9488_HEIGHT) py = ILI9488_HEIGHT - 1;

        pt->x = (uint16_t)px;
        pt->y = (uint16_t)py;
    }
//        else {
//        /* Uncalibrated fallback: simple linear remap.
//         * Typical XPT2046 raw range: ~200–3900.
//         * Adjust MIN/MAX for your specific panel if needed.
//         */
//        #define TOUCH_RAW_X_MIN  200u
//        #define TOUCH_RAW_X_MAX  3900u
//        #define TOUCH_RAW_Y_MIN  200u
//        #define TOUCH_RAW_Y_MAX  3900u
//
//        if (rawX < TOUCH_RAW_X_MIN) rawX = TOUCH_RAW_X_MIN;
//        if (rawX > TOUCH_RAW_X_MAX) rawX = TOUCH_RAW_X_MAX;
//        if (rawY < TOUCH_RAW_Y_MIN) rawY = TOUCH_RAW_Y_MIN;
//        if (rawY > TOUCH_RAW_Y_MAX) rawY = TOUCH_RAW_Y_MAX;
//
//        pt->x = (uint16_t)(((uint32_t)(rawX - TOUCH_RAW_X_MIN) * ILI9488_WIDTH)
//                            / (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN));
//        pt->y = (uint16_t)(((uint32_t)(rawY - TOUCH_RAW_Y_MIN) * ILI9488_HEIGHT)
//                            / (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN));
//    }
    else {
        #define TOUCH_RAW_X_MIN  200u
        #define TOUCH_RAW_X_MAX  3900u
        #define TOUCH_RAW_Y_MIN  200u
        #define TOUCH_RAW_Y_MAX  3900u

        if (rawX < TOUCH_RAW_X_MIN) rawX = TOUCH_RAW_X_MIN;
        if (rawX > TOUCH_RAW_X_MAX) rawX = TOUCH_RAW_X_MAX;
        if (rawY < TOUCH_RAW_Y_MIN) rawY = TOUCH_RAW_Y_MIN;
        if (rawY > TOUCH_RAW_Y_MAX) rawY = TOUCH_RAW_Y_MAX;

        /* Landscape: XPT2046 raw-X is the display's Y axis, raw-Y is display's X axis.
         * Both are inverted relative to MADCTL_MV|MX orientation.
         * If one axis is still mirrored after flashing, remove just the
         * "(ILI9488_WIDTH-1u) -" or "(ILI9488_HEIGHT-1u) -" for that axis. */

        /* rawY → pixel X, inverted */
        pt->x = (ILI9488_WIDTH - 1u) -
                (uint16_t)(((uint32_t)(rawY - TOUCH_RAW_Y_MIN) * ILI9488_WIDTH)
                           / (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN));

        /* rawX → pixel Y, inverted */
        pt->y = (ILI9488_HEIGHT - 1u) -
                (uint16_t)(((uint32_t)(rawX - TOUCH_RAW_X_MIN) * ILI9488_HEIGHT)
                           / (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN));
    }

    return true;
}

void XPT2046_SetCalibration(const XPT2046_Calib_t *cal)
{
    if (cal) {
        memcpy(&_calib, cal, sizeof(_calib));
    }
}

void XPT2046_GetCalibration(XPT2046_Calib_t *cal)
{
    if (cal) {
        memcpy(cal, &_calib, sizeof(_calib));
    }
}

void XPT2046_IRQHandler(void)
{
    _touched = true;
}



/**
 ******************************************************************************
 * @file    xpt2046.c
 * @brief   XPT2046 Resistive Touchscreen Driver
 *          Shares SPI1 bus with ILI9488.  Uses separate TOUCH_CS (PF2).
 *          TOUCH_INT (PF3) is configured as EXTI rising-edge in CubeMX.
 *
 * SPI timing requirements for XPT2046:
 *   • CPOL=0, CPHA=0  (SPI Mode 0) – matches display, so same SPI config OK
 *   • Max DCLK = 2 MHz for accurate ADC readings.
 *   • STM32F429 SPI1 at 180/2 = 90 MHz → use SPI prescaler 64 = 1.4 MHz
 *     when sampling touch; switch back to /2 for display transfers.
 *     (Alternatively use a software SPI or ensure your SPI prescaler is ≤32.)
 *     The driver handles prescaler switching transparently.
 *
 * Calibration:
 *   The raw XPT2046 ADC outputs (0-4095) must be mapped to pixel coords.
 *   A 3-point affine transform is used.  Store the XPT2046_Calib_t in
 *   non-volatile memory (e.g. last sector of flash) and call
 *   XPT2046_SetCalibration() on boot.
 *
 *   Default (uncalibrated) fall-back uses simple linear mapping with
 *   axis swap and inversion to match ILI9488 landscape orientation.
 ******************************************************************************
 */






//
//
//#include "xpt2046.h"
//#include "ili9488.h"   /* For ILI9488_WIDTH / HEIGHT, CS helpers */
//#include <string.h>
//
///* ─── Private state ─────────────────────────────────────────────────────── */
//static SPI_HandleTypeDef *_hspi    = NULL;
//static volatile bool      _touched = false;
//static XPT2046_Calib_t    _calib   = { .valid = false };
//
///* ─── XPT2046 control byte: channel select + mode ──────────────────────── */
//#define XPT2046_CMD_X    0xD0u   /* 12-bit, differential, X  */
//#define XPT2046_CMD_Y    0x90u   /* 12-bit, differential, Y  */
//#define XPT2046_CMD_Z1   0xB0u   /* pressure measurement     */
//#define XPT2046_CMD_Z2   0xC0u
//
///* Number of samples to average (reduces noise) */
//#define XPT2046_AVG_SAMPLES  4u
//
///* Touch pressure threshold (higher Z1-Z2 ratio = more pressure) */
//#define XPT2046_Z_THRESHOLD  200u
//
///* ─── Raw ADC limits for uncalibrated fallback ──────────────────────────── */
///* Touch a corner and read the snprintf debug output to tune these for your  */
///* specific panel if the fallback mapping is slightly off.                   */
//#define TOUCH_RAW_X_MIN  200u
//#define TOUCH_RAW_X_MAX  3900u
//#define TOUCH_RAW_Y_MIN  200u
//#define TOUCH_RAW_Y_MAX  3900u
//
///* ─── SPI prescaler switching ───────────────────────────────────────────── */
///* SPI1_CR1 BaudRate field bits [5:3] – prescaler divider value              */
//#define SPI_BAUDRATE_DIV2    0x00u
//#define SPI_BAUDRATE_DIV64   0x05u
//
//static void _SPI_SetPrescaler(uint8_t prescalerBits)
//{
//    /* Disable SPI, change prescaler, re-enable */
//    _hspi->Instance->CR1 &= ~SPI_CR1_SPE;
//    _hspi->Instance->CR1 &= ~SPI_CR1_BR_Msk;
//    _hspi->Instance->CR1 |= (uint32_t)prescalerBits << SPI_CR1_BR_Pos;
//    _hspi->Instance->CR1 |=  SPI_CR1_SPE;
//}
//
///* ─── Low-level SPI for touch (slow) ───────────────────────────────────── */
//
//static inline void _TOUCH_CS_Assert(void)
//{
//    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET);
//}
//static inline void _TOUCH_CS_Deassert(void)
//{
//    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
//}
//
///**
// * @brief  Read one 12-bit sample from the XPT2046 for the given command.
// *         CS must already be asserted.
// */
//static uint16_t _ReadChannel(uint8_t cmd)
//{
//    uint8_t tx[3] = { cmd, 0x00, 0x00 };
//    uint8_t rx[3] = { 0 };
//    HAL_SPI_TransmitReceive(_hspi, tx, rx, 3, HAL_MAX_DELAY);
//    /* Result: rx[1] bits[6:0] << 5 | rx[2] bits[7:3] >> 3 → 12 bits */
//    return (uint16_t)(((uint16_t)rx[1] << 4u) | ((uint16_t)rx[2] >> 4u));
//}
//
///**
// * @brief  Average N samples for one channel.
// */
//static uint16_t _SampleAvg(uint8_t cmd)
//{
//    uint32_t acc = 0;
//    for (uint8_t i = 0; i < XPT2046_AVG_SAMPLES; i++) {
//        acc += _ReadChannel(cmd);
//    }
//    return (uint16_t)(acc / XPT2046_AVG_SAMPLES);
//}
//
///* ─── Public API ─────────────────────────────────────────────────────────── */
//
//void XPT2046_Init(SPI_HandleTypeDef *hspi)
//{
//    _hspi = hspi;
//    _TOUCH_CS_Deassert();
//    _touched = false;
//    memset(&_calib, 0, sizeof(_calib));
//    _calib.valid = false;
//}
//
//bool XPT2046_IsTouched(void)
//{
//    /* Quick check via INT pin state (active LOW = touched) */
//    return (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);
//}
//
//bool XPT2046_GetTouchRaw(uint16_t *rawX, uint16_t *rawY, uint16_t *rawZ)
//{
//    /* Switch SPI to slow speed for ADC accuracy */
//    _SPI_SetPrescaler(SPI_BAUDRATE_DIV64);
//
//    _TOUCH_CS_Assert();
//
//    /* Measure pressure first – discard if nothing is touching */
//    uint16_t z1 = _ReadChannel(XPT2046_CMD_Z1);
//    uint16_t z2 = _ReadChannel(XPT2046_CMD_Z2);
//
//    /* Pressure = Z1/4096 - Z2/4096 + 1  (simplified: z1 - z2 + 4096) */
//    int32_t pressure = (int32_t)z1 - (int32_t)z2 + 4096;
//
//    if (pressure < (int32_t)XPT2046_Z_THRESHOLD) {
//        _TOUCH_CS_Deassert();
//        _SPI_SetPrescaler(SPI_BAUDRATE_DIV2);
//        return false;
//    }
//
//    uint16_t x = _SampleAvg(XPT2046_CMD_X);
//    uint16_t y = _SampleAvg(XPT2046_CMD_Y);
//
//    _TOUCH_CS_Deassert();
//    _SPI_SetPrescaler(SPI_BAUDRATE_DIV2);
//
//    if (rawX) *rawX = x;
//    if (rawY) *rawY = y;
//    if (rawZ) *rawZ = (uint16_t)(pressure > 4095 ? 4095 : pressure);
//
//    return true;
//}
//
//bool XPT2046_GetTouchPixel(XPT2046_Point_t *pt)
//{
//    if (!pt) return false;
//
//    uint16_t rawX, rawY, rawZ;
//    if (!XPT2046_GetTouchRaw(&rawX, &rawY, &rawZ)) return false;
//
//    pt->z = rawZ;
//
//    if (_calib.valid) {
//        /* Affine transform:
//         *   pixel_x = (alphaX * rawX + betaX) / divider
//         *   pixel_y = (alphaY * rawY + betaY) / divider
//         */
//        int32_t px = (_calib.alphaX * (int32_t)rawX + _calib.betaX) / _calib.divider;
//        int32_t py = (_calib.alphaY * (int32_t)rawY + _calib.betaY) / _calib.divider;
//
//        /* Clamp to display bounds */
//        if (px < 0) px = 0;
//        if (py < 0) py = 0;
//        if (px >= (int32_t)ILI9488_WIDTH)  px = ILI9488_WIDTH  - 1;
//        if (py >= (int32_t)ILI9488_HEIGHT) py = ILI9488_HEIGHT - 1;
//
//        pt->x = (uint16_t)px;
//        pt->y = (uint16_t)py;
//
//    } else {
//        /*
//         * Uncalibrated fallback – landscape ILI9488 + XPT2046 fix:
//         *
//         * The resistive panel's raw X/Y axes are physically swapped relative
//         * to the display in landscape mode, and both run in the opposite
//         * direction.  We therefore:
//         *   1. Swap rawX ↔ rawY so they map to the correct display axis.
//         *   2. Invert both axes (subtract from max) so the direction matches.
//         *
//         * If one axis is still mirrored after flashing, remove just the
//         * "(ILI9488_WIDTH - 1u) -" or "(ILI9488_HEIGHT - 1u) -" for that axis.
//         *
//         * Tune TOUCH_RAW_*_MIN/MAX at the top of this file using the raw
//         * values printed by the snprintf debug line in main.c.
//         */
//
//        /* Clamp raw values to known panel range */
//        if (rawX < TOUCH_RAW_X_MIN) rawX = TOUCH_RAW_X_MIN;
//        if (rawX > TOUCH_RAW_X_MAX) rawX = TOUCH_RAW_X_MAX;
//        if (rawY < TOUCH_RAW_Y_MIN) rawY = TOUCH_RAW_Y_MIN;
//        if (rawY > TOUCH_RAW_Y_MAX) rawY = TOUCH_RAW_Y_MAX;
//
//        /* rawY → pixel X  (axis swap), inverted */
//        pt->x = (ILI9488_WIDTH - 1u) -
//                (uint16_t)(((uint32_t)(rawY - TOUCH_RAW_Y_MIN) * ILI9488_WIDTH)
//                           / (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN));
//
//        /* rawX → pixel Y  (axis swap), inverted */
//        pt->y = (ILI9488_HEIGHT - 1u) -
//                (uint16_t)(((uint32_t)(rawX - TOUCH_RAW_X_MIN) * ILI9488_HEIGHT)
//                           / (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN));
//    }
//
//    return true;
//}
//
//void XPT2046_SetCalibration(const XPT2046_Calib_t *cal)
//{
//    if (cal) {
//        memcpy(&_calib, cal, sizeof(_calib));
//    }
//}
//
//void XPT2046_GetCalibration(XPT2046_Calib_t *cal)
//{
//    if (cal) {
//        memcpy(cal, &_calib, sizeof(_calib));
//    }
//}
//
//void XPT2046_IRQHandler(void)
//{
//    _touched = true;
//}
