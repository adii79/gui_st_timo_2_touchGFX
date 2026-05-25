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
 *     The driver handles prescaler switching transparently.
 *
 * Calibration:
 *   The raw XPT2046 ADC outputs (0-4095) must be mapped to pixel coords.
 *   A 3-point affine transform is used.  Store the XPT2046_Calib_t in
 *   non-volatile memory (e.g. last sector of flash) and call
 *   XPT2046_SetCalibration() on boot.
 *
 *   Default (uncalibrated) fall-back uses the tunable defines below.
 ******************************************************************************
 */

#include "xpt2046.h"
#include "ili9488.h"   /* For ILI9488_WIDTH / HEIGHT */
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
   TOUCH TUNING  — the ONLY section you ever need to edit
   ═══════════════════════════════════════════════════════════════════════════

   HOW TO READ THE RAW LIMITS (one-time, using the yellow debug string):
   ──────────────────────────────────────────────────────────────────────
   Touch each of the 4 corners and note the  raw(XXXX,YYYY)  readout.

     TOUCH_RAW_X_MIN  = smallest  rawX seen across all corners
     TOUCH_RAW_X_MAX  = largest   rawX seen across all corners
     TOUCH_RAW_Y_MIN  = smallest  rawY seen across all corners
     TOUCH_RAW_Y_MAX  = largest   rawY seen across all corners

   From the video of this board the measured corners gave approximately:
     rawX range  :  28 – 1800
     rawY range  : 300 – 2047

   HOW TO USE THE OFFSET TRIM (after raw limits are set):
   ──────────────────────────────────────────────────────
   Touch the exact centre of the screen and read X / Y pixel values.
   Screen centre for 480×320 landscape = pixel (240, 160).

   If the crosshair shows  X:230  instead of X:240  →  TOUCH_OFFSET_X = +10
   If the crosshair shows  X:255  instead of X:240  →  TOUCH_OFFSET_X = -15
   Same logic for Y.  Range: -99 … +99  (signed, pixels).

   AXIS ORIENTATION (LANDSCAPE  MADCTL = MV|MX|MY|BGR):
   ──────────────────────────────────────────────────────
   TOUCH_SWAP_XY  = 1   (panel X wire → display Y, panel Y wire → display X)
   TOUCH_INVERT_X = 0
   TOUCH_INVERT_Y = 0
   ═══════════════════════════════════════════════════════════════════════════ */

/* ── Raw ADC limits  (tune to your actual corner readings) ─────────────── */
#define TOUCH_RAW_X_MIN    28u    /* smallest rawX at any corner            */
#define TOUCH_RAW_X_MAX  1800u    /* largest  rawX at any corner            */
#define TOUCH_RAW_Y_MIN   300u    /* smallest rawY at any corner            */
#define TOUCH_RAW_Y_MAX  2047u    /* largest  rawY at any corner            */

/* ── Axis orientation ──────────────────────────────────────────────────── */
#define TOUCH_SWAP_XY      1u     /* 1 = swap raw X↔Y before mapping        */
#define TOUCH_INVERT_X     0u     /* 1 = mirror pixel X (flip left/right)   */
#define TOUCH_INVERT_Y     0u     /* 1 = mirror pixel Y (flip top/bottom)   */

/* ── Fine offset trim  (signed pixels, applied AFTER mapping) ──────────── *
 *   Touch screen centre → read pixel X,Y → set offset = expected - actual  *
 *   Example: centre should be (240,160), reads (225,160) → OFFSET_X = +15  */
#define TOUCH_OFFSET_X     +30      /* signed int, pixels, range ~ -99..+99   */
#define TOUCH_OFFSET_Y     0      /* signed int, pixels, range ~ -99..+99   */

/* ═══════════════════════════════════════════════════════════════════════════
   END OF TUNING SECTION — do not edit below unless you know what you're doing
   ═══════════════════════════════════════════════════════════════════════════ */


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

/* Touch pressure threshold */
#define XPT2046_Z_THRESHOLD  200u

/* ─── SPI prescaler switching ───────────────────────────────────────────── */
#define SPI_BAUDRATE_DIV2    0x00u
#define SPI_BAUDRATE_DIV64   0x05u

static void _SPI_SetPrescaler(uint8_t prescalerBits)
{
    _hspi->Instance->CR1 &= ~SPI_CR1_SPE;
    _hspi->Instance->CR1 &= ~SPI_CR1_BR_Msk;
    _hspi->Instance->CR1 |= (uint32_t)prescalerBits << SPI_CR1_BR_Pos;
    _hspi->Instance->CR1 |=  SPI_CR1_SPE;
}

/* ─── CS helpers ────────────────────────────────────────────────────────── */
static inline void _TOUCH_CS_Assert(void)
{
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET);
}
static inline void _TOUCH_CS_Deassert(void)
{
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
}

/* ─── Low-level SPI read ────────────────────────────────────────────────── */
static uint16_t _ReadChannel(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };
    HAL_SPI_TransmitReceive(_hspi, tx, rx, 3, HAL_MAX_DELAY);
    return (uint16_t)(((uint16_t)rx[1] << 4u) | ((uint16_t)rx[2] >> 4u));
}

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
    return (HAL_GPIO_ReadPin(TOUCH_INT_GPIO_Port, TOUCH_INT_Pin) == GPIO_PIN_RESET);
}

bool XPT2046_GetTouchRaw(uint16_t *rawX, uint16_t *rawY, uint16_t *rawZ)
{
    _SPI_SetPrescaler(SPI_BAUDRATE_DIV64);
    _TOUCH_CS_Assert();

    uint16_t z1 = _ReadChannel(XPT2046_CMD_Z1);
    uint16_t z2 = _ReadChannel(XPT2046_CMD_Z2);

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
        /* ── 3-point affine calibration (set via XPT2046_SetCalibration) ── */
        int32_t px = (_calib.alphaX * (int32_t)rawX + _calib.betaX) / _calib.divider;
        int32_t py = (_calib.alphaY * (int32_t)rawY + _calib.betaY) / _calib.divider;

        /* Apply fine offset trim */
        px += TOUCH_OFFSET_X;
        py += TOUCH_OFFSET_Y;

        /* Clamp to display bounds */
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px >= (int32_t)ILI9488_WIDTH)  px = ILI9488_WIDTH  - 1;
        if (py >= (int32_t)ILI9488_HEIGHT) py = ILI9488_HEIGHT - 1;

        pt->x = (uint16_t)px;
        pt->y = (uint16_t)py;

    } else {
        /* ── Uncalibrated fallback — controlled entirely by tuning defines ─
         *
         * Pipeline:
         *   1. Clamp raw reading to [MIN, MAX]
         *   2. Optional axis swap (TOUCH_SWAP_XY)
         *   3. Linear map to [0, screen_size-1]
         *   4. Optional axis inversion (TOUCH_INVERT_X / TOUCH_INVERT_Y)
         *   5. Fine pixel offset (TOUCH_OFFSET_X / TOUCH_OFFSET_Y)
         *   6. Clamp result to display bounds
         * ─────────────────────────────────────────────────────────────── */

        /* Step 1: Clamp raw to panel's physical range */
        if (rawX < TOUCH_RAW_X_MIN) rawX = TOUCH_RAW_X_MIN;
        if (rawX > TOUCH_RAW_X_MAX) rawX = TOUCH_RAW_X_MAX;
        if (rawY < TOUCH_RAW_Y_MIN) rawY = TOUCH_RAW_Y_MIN;
        if (rawY > TOUCH_RAW_Y_MAX) rawY = TOUCH_RAW_Y_MAX;

        /* Step 2: Axis swap — choose which raw feeds which pixel axis */
#if TOUCH_SWAP_XY
        /* Panel rawY  →  pixel X axis,  panel rawX  →  pixel Y axis */
        uint16_t axX     = rawY;
        uint16_t axY     = rawX;
        uint16_t axX_min = TOUCH_RAW_Y_MIN;
        uint16_t axX_max = TOUCH_RAW_Y_MAX;
        uint16_t axY_min = TOUCH_RAW_X_MIN;
        uint16_t axY_max = TOUCH_RAW_X_MAX;
#else
        /* Panel rawX  →  pixel X axis,  panel rawY  →  pixel Y axis */
        uint16_t axX     = rawX;
        uint16_t axY     = rawY;
        uint16_t axX_min = TOUCH_RAW_X_MIN;
        uint16_t axX_max = TOUCH_RAW_X_MAX;
        uint16_t axY_min = TOUCH_RAW_Y_MIN;
        uint16_t axY_max = TOUCH_RAW_Y_MAX;
#endif

        /* Step 3: Linear map to full pixel range
         *   pixel = (raw - min) * screen_size / (max - min)
         *   Result covers [0 … screen_size-1] end-to-end. */
        int32_t px = (int32_t)(((uint32_t)(axX - axX_min) * (uint32_t)(ILI9488_WIDTH))
                               / (uint32_t)(axX_max - axX_min));
        int32_t py = (int32_t)(((uint32_t)(axY - axY_min) * (uint32_t)(ILI9488_HEIGHT))
                               / (uint32_t)(axY_max - axY_min));

        /* Step 4: Axis inversion */
#if TOUCH_INVERT_X
        px = (int32_t)(ILI9488_WIDTH  - 1u) - px;
#endif
#if TOUCH_INVERT_Y
        py = (int32_t)(ILI9488_HEIGHT - 1u) - py;
#endif

        /* Step 5: Fine signed pixel offset trim */
        px += (int32_t)TOUCH_OFFSET_X;
        py += (int32_t)TOUCH_OFFSET_Y;

        /* Step 6: Clamp to valid display range */
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px >= (int32_t)ILI9488_WIDTH)  px = (int32_t)(ILI9488_WIDTH  - 1u);
        if (py >= (int32_t)ILI9488_HEIGHT) py = (int32_t)(ILI9488_HEIGHT - 1u);

        pt->x = (uint16_t)px;
        pt->y = (uint16_t)py;
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
