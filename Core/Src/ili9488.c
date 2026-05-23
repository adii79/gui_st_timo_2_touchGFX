/**
 ******************************************************************************
 * @file    ili9488.c
 * @brief   ILI9488 3.5" SPI TFT Display Driver – full implementation
 *          Controller : ILI9488   Resolution : 480 × 320
 *          Colour fmt : RGB666 (18-bit, 3 bytes/pixel over 4-wire SPI)
 *          MCU        : STM32F429ZGTx  @180 MHz
 *          Interface  : SPI1, software CS / DC / RST via GPIO
 ******************************************************************************
 */

#include "ili9488.h"
#include <stdlib.h>   /* abs() */
#include <string.h>   /* memset */

/* ─── Integer square root (no <math.h> / -lm needed) ───────────────────── */
static uint16_t _isqrt(uint32_t n)
{
    if (n == 0u) return 0u;
    uint32_t x = n, y = 1u;
    while (x > y) { x = (x + y) / 2u; y = n / x; }
    return (uint16_t)x;
}

/* ─── Private handle ────────────────────────────────────────────────────── */
static SPI_HandleTypeDef *_hspi = NULL;

/* ─── Current display dimensions (swap on rotation) ────────────────────── */
static uint16_t _width  = ILI9488_WIDTH;
static uint16_t _height = ILI9488_HEIGHT;

/* ─────────────────────────────────────────────────────────────────────────
   ILI9488 Command set
   ───────────────────────────────────────────────────────────────────────── */
#define ILI9488_CMD_NOP             0x00u
#define ILI9488_CMD_SWRESET         0x01u
#define ILI9488_CMD_SLPIN           0x10u
#define ILI9488_CMD_SLPOUT          0x11u
#define ILI9488_CMD_INVOFF          0x20u
#define ILI9488_CMD_INVON           0x21u
#define ILI9488_CMD_DISPOFF         0x28u
#define ILI9488_CMD_DISPON          0x29u
#define ILI9488_CMD_CASET           0x2Au
#define ILI9488_CMD_PASET           0x2Bu
#define ILI9488_CMD_RAMWR           0x2Cu
#define ILI9488_CMD_MADCTL          0x36u
#define ILI9488_CMD_COLMOD          0x3Au
#define ILI9488_CMD_PGAMCTRL        0xE0u
#define ILI9488_CMD_NGAMCTRL        0xE1u
#define ILI9488_CMD_PWRCTRL1        0xC0u
#define ILI9488_CMD_PWRCTRL2        0xC1u
#define ILI9488_CMD_VMCTRL1         0xC5u
#define ILI9488_CMD_FRMCTR1         0xB1u
#define ILI9488_CMD_INVCTR          0xB4u
#define ILI9488_CMD_DFUNCTR         0xB6u
#define ILI9488_CMD_IMGFUNCT        0xE9u
#define ILI9488_CMD_ADJCTR3         0xF7u

/* MADCTL bits */
#define MADCTL_MY  0x80u
#define MADCTL_MX  0x40u
#define MADCTL_MV  0x20u
#define MADCTL_ML  0x10u
#define MADCTL_BGR 0x08u
#define MADCTL_MH  0x04u

/* COLMOD pixel format: 18 bpp (RGB666) */
#define ILI9488_COLMOD_18BPP  0x66u

/* ─────────────────────────────────────────────────────────────────────────
   Low-level GPIO helpers
   ───────────────────────────────────────────────────────────────────────── */
static inline void _DC_Cmd(void)   { HAL_GPIO_WritePin(DISPL_DC_GPIO_Port,  DISPL_DC_Pin,  GPIO_PIN_RESET); }
static inline void _DC_Data(void)  { HAL_GPIO_WritePin(DISPL_DC_GPIO_Port,  DISPL_DC_Pin,  GPIO_PIN_SET);   }
static inline void _RST_Low(void)  { HAL_GPIO_WritePin(DISPL_RST_GPIO_Port, DISPL_RST_Pin, GPIO_PIN_RESET); }
static inline void _RST_High(void) { HAL_GPIO_WritePin(DISPL_RST_GPIO_Port, DISPL_RST_Pin, GPIO_PIN_SET);   }
static inline void _LED_On(void)   { HAL_GPIO_WritePin(DISPL_LED_GPIO_Port, DISPL_LED_Pin, GPIO_PIN_SET);   }
static inline void _LED_Off(void)  { HAL_GPIO_WritePin(DISPL_LED_GPIO_Port, DISPL_LED_Pin, GPIO_PIN_RESET); }

void ILI9488_CS_Assert(void)   { HAL_GPIO_WritePin(DISPL_CS_GPIO_Port, DISPL_CS_Pin, GPIO_PIN_RESET); }
void ILI9488_CS_Deassert(void) { HAL_GPIO_WritePin(DISPL_CS_GPIO_Port, DISPL_CS_Pin, GPIO_PIN_SET);   }

/* ─────────────────────────────────────────────────────────────────────────
   Low-level SPI transfers
   ───────────────────────────────────────────────────────────────────────── */

static void _WriteCmd(uint8_t cmd)
{
    _DC_Cmd();
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
}

static void _WriteDataBuf(const uint8_t *buf, uint32_t len)
{
    _DC_Data();
    HAL_SPI_Transmit(_hspi, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

static void _WriteReg(uint8_t cmd, const uint8_t *params, uint8_t nparams)
{
    ILI9488_CS_Assert();
    _WriteCmd(cmd);
    if (nparams && params) {
        _WriteDataBuf(params, nparams);
    }
    ILI9488_CS_Deassert();
}

/* ─────────────────────────────────────────────────────────────────────────
   Window / pixel helpers
   ───────────────────────────────────────────────────────────────────────── */

void ILI9488_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buf[4];

    buf[0] = (uint8_t)(x0 >> 8); buf[1] = (uint8_t)(x0);
    buf[2] = (uint8_t)(x1 >> 8); buf[3] = (uint8_t)(x1);
    _WriteReg(ILI9488_CMD_CASET, buf, 4);

    buf[0] = (uint8_t)(y0 >> 8); buf[1] = (uint8_t)(y0);
    buf[2] = (uint8_t)(y1 >> 8); buf[3] = (uint8_t)(y1);
    _WriteReg(ILI9488_CMD_PASET, buf, 4);

    ILI9488_CS_Assert();
    _WriteCmd(ILI9488_CMD_RAMWR);
    _DC_Data();
    /* CS stays asserted – caller sends pixels then deasserts */
}

void ILI9488_WritePixels_RGB666(const uint8_t *buf, uint32_t byteCount)
{
    while (byteCount > 0u) {
        uint32_t chunk = (byteCount > 65535u) ? 65535u : byteCount;
        HAL_SPI_Transmit(_hspi, (uint8_t *)buf, (uint16_t)chunk, HAL_MAX_DELAY);
        buf       += chunk;
        byteCount -= chunk;
    }
    ILI9488_CS_Deassert();
}

void ILI9488_WritePixels_RGB565(const uint16_t *buf, uint32_t count)
{
    #define BATCH 64u
    uint8_t tmp[BATCH * 3u];
    uint32_t idx = 0u;

    while (count > 0u) {
        uint32_t batch = (count > BATCH) ? BATCH : count;
        for (uint32_t i = 0u; i < batch; i++) {
            uint16_t c = buf[idx++];
            tmp[i*3u+0u] = RGB565_TO_R6(c);
            tmp[i*3u+1u] = RGB565_TO_G6(c);
            tmp[i*3u+2u] = RGB565_TO_B6(c);
        }
        HAL_SPI_Transmit(_hspi, tmp, (uint16_t)(batch * 3u), HAL_MAX_DELAY);
        count -= batch;
    }
    ILI9488_CS_Deassert();
    #undef BATCH
}

/* ─────────────────────────────────────────────────────────────────────────
   Initialisation sequence
   ───────────────────────────────────────────────────────────────────────── */
//void ILI9488_Init(SPI_HandleTypeDef *hspi)
//{
//    _hspi = hspi;
//
//    ILI9488_CS_Deassert();
//    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);
//
//    _LED_Off();
//    _RST_High(); HAL_Delay(5);
//    _RST_Low();  HAL_Delay(20);
//    _RST_High(); HAL_Delay(150);
//
//    { static const uint8_t p[] = {0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F};
//      _WriteReg(ILI9488_CMD_PGAMCTRL, p, sizeof(p)); }
//
//    { static const uint8_t p[] = {0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F};
//      _WriteReg(ILI9488_CMD_NGAMCTRL, p, sizeof(p)); }
//
//    { static const uint8_t p[] = {0x17, 0x15}; _WriteReg(ILI9488_CMD_PWRCTRL1,  p, 2); }
//    { static const uint8_t p[] = {0x41};        _WriteReg(ILI9488_CMD_PWRCTRL2,  p, 1); }
//    { static const uint8_t p[] = {0x00,0x12,0x80}; _WriteReg(ILI9488_CMD_VMCTRL1, p, 3); }
////    { static const uint8_t p[] = {MADCTL_MX | MADCTL_BGR}; _WriteReg(ILI9488_CMD_MADCTL,  p, 1);}
////    { static const uint8_t p[] = {MADCTL_MV | MADCTL_MX | MADCTL_BGR}; _WriteReg(ILI9488_CMD_MADCTL, p, 1); }
//    { static const uint8_t p[] = {MADCTL_MV | MADCTL_MY | MADCTL_BGR}; _WriteReg(ILI9488_CMD_MADCTL, p, 1); }
//    { static const uint8_t p[] = {ILI9488_COLMOD_18BPP};   _WriteReg(ILI9488_CMD_COLMOD,  p, 1); }
//    { static const uint8_t p[] = {0x00}; _WriteReg(0xB0, p, 1); }
//    { static const uint8_t p[] = {0xA0}; _WriteReg(ILI9488_CMD_FRMCTR1, p, 1); }
//    { static const uint8_t p[] = {0x02}; _WriteReg(ILI9488_CMD_INVCTR,  p, 1); }
//    { static const uint8_t p[] = {0x02,0x02,0x3B}; _WriteReg(ILI9488_CMD_DFUNCTR, p, 3); }
//    { static const uint8_t p[] = {0x00}; _WriteReg(ILI9488_CMD_IMGFUNCT, p, 1); }
//    { static const uint8_t p[] = {0xA9,0x51,0x2C,0x02}; _WriteReg(ILI9488_CMD_ADJCTR3, p, 4); }
//
//    _WriteReg(ILI9488_CMD_SLPOUT, NULL, 0); HAL_Delay(120);
//    _WriteReg(ILI9488_CMD_DISPON, NULL, 0); HAL_Delay(20);
//
//    _LED_On();
//    ILI9488_FillScreen(ILI9488_COLOR_BLACK);
//}

void ILI9488_Init(SPI_HandleTypeDef *hspi)
{
    _hspi = hspi;

    ILI9488_CS_Deassert();
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);

    _LED_Off();
    _RST_High(); HAL_Delay(5);
    _RST_Low();  HAL_Delay(20);
    _RST_High(); HAL_Delay(150);

    { static const uint8_t p[] = {0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F};
      _WriteReg(ILI9488_CMD_PGAMCTRL, p, sizeof(p)); }
    { static const uint8_t p[] = {0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F};
      _WriteReg(ILI9488_CMD_NGAMCTRL, p, sizeof(p)); }

    { static const uint8_t p[] = {0x17, 0x15}; _WriteReg(ILI9488_CMD_PWRCTRL1,  p, 2); }
    { static const uint8_t p[] = {0x41};        _WriteReg(ILI9488_CMD_PWRCTRL2,  p, 1); }
    { static const uint8_t p[] = {0x00,0x12,0x80}; _WriteReg(ILI9488_CMD_VMCTRL1, p, 3); }

    /* *** CHANGED: MV+MX+MY+BGR = landscape, no X mirror *** */
    { static const uint8_t p[] = {MADCTL_MV | MADCTL_MX | MADCTL_MY | MADCTL_BGR};
      _WriteReg(ILI9488_CMD_MADCTL, p, 1); }

    { static const uint8_t p[] = {ILI9488_COLMOD_18BPP};   _WriteReg(ILI9488_CMD_COLMOD,  p, 1); }
    { static const uint8_t p[] = {0x00}; _WriteReg(0xB0, p, 1); }
    { static const uint8_t p[] = {0xA0}; _WriteReg(ILI9488_CMD_FRMCTR1, p, 1); }
    { static const uint8_t p[] = {0x02}; _WriteReg(ILI9488_CMD_INVCTR,  p, 1); }
    { static const uint8_t p[] = {0x02,0x02,0x3B}; _WriteReg(ILI9488_CMD_DFUNCTR, p, 3); }
    { static const uint8_t p[] = {0x00}; _WriteReg(ILI9488_CMD_IMGFUNCT, p, 1); }
    { static const uint8_t p[] = {0xA9,0x51,0x2C,0x02}; _WriteReg(ILI9488_CMD_ADJCTR3, p, 4); }

    _WriteReg(ILI9488_CMD_SLPOUT, NULL, 0); HAL_Delay(120);
    _WriteReg(ILI9488_CMD_DISPON, NULL, 0); HAL_Delay(20);

    _LED_On();
    ILI9488_FillScreen(ILI9488_COLOR_BLACK);
}

/* ─────────────────────────────────────────────────────────────────────────
   Orientation
   ───────────────────────────────────────────────────────────────────────── */
//void ILI9488_SetOrientation(ILI9488_Orientation_t orient)
//{
//    uint8_t madctl = MADCTL_BGR;
//    switch (orient) {
//        default:
////        case ILI9488_ORIENT_LANDSCAPE:
////            madctl |= MADCTL_MX; _width = ILI9488_WIDTH;  _height = ILI9488_HEIGHT; break;
////        case ILI9488_ORIENT_PORTRAIT:
////            madctl |= MADCTL_MV; _width = ILI9488_HEIGHT; _height = ILI9488_WIDTH;  break;
////        case ILI9488_ORIENT_LANDSCAPE_FLIP:
////            madctl |= MADCTL_MY; _width = ILI9488_WIDTH;  _height = ILI9488_HEIGHT; break;
////        case ILI9488_ORIENT_PORTRAIT_FLIP:
////            madctl |= (MADCTL_MX|MADCTL_MY|MADCTL_MV); _width = ILI9488_HEIGHT; _height = ILI9488_WIDTH; break;
//        	// AFTER
//        case ILI9488_ORIENT_LANDSCAPE:
//            madctl |= MADCTL_MV | MADCTL_MX;
//            _width = 480u; _height = 320u; break;
//        	case ILI9488_ORIENT_PORTRAIT:            // 320 wide, 480 tall
//        	    madctl |= 0u;  /* no MV, no MX */
//        	    _width = 320u; _height = 480u; break;
//        	 case ILI9488_ORIENT_LANDSCAPE_FLIP:
//        	            madctl |= MADCTL_MV | MADCTL_MY;
//        	            _width = 480u; _height = 320u; break;
//        	case ILI9488_ORIENT_PORTRAIT_FLIP:       // 320 wide, 480 tall, flipped
//        	    madctl |= MADCTL_MX | MADCTL_MY;
//        	    _width = 320u; _height = 480u; break;
//    }
//    _WriteReg(ILI9488_CMD_MADCTL, &madctl, 1);
//}

void ILI9488_SetOrientation(ILI9488_Orientation_t orient)
{
    uint8_t madctl = MADCTL_BGR;
    switch (orient) {
        default:
        case ILI9488_ORIENT_LANDSCAPE:           /* 480 wide, 320 tall – normal */
            madctl |= MADCTL_MV | MADCTL_MX | MADCTL_MY;
            _width = 480u; _height = 320u;
            break;
        case ILI9488_ORIENT_LANDSCAPE_FLIP:      /* 480 wide, 320 tall – 180° flip */
            madctl |= MADCTL_MV;
            _width = 480u; _height = 320u;
            break;
        case ILI9488_ORIENT_PORTRAIT:            /* 320 wide, 480 tall – normal */
            madctl |= MADCTL_MX;
            _width = 320u; _height = 480u;
            break;
        case ILI9488_ORIENT_PORTRAIT_FLIP:       /* 320 wide, 480 tall – 180° flip */
            madctl |= MADCTL_MY;
            _width = 320u; _height = 480u;
            break;
    }
    _WriteReg(ILI9488_CMD_MADCTL, &madctl, 1);
}

/* ─────────────────────────────────────────────────────────────────────────
   Backlight
   ───────────────────────────────────────────────────────────────────────── */
void ILI9488_SetBacklight(bool on)
{
    if (on) _LED_On(); else _LED_Off();
}

/* ─────────────────────────────────────────────────────────────────────────
   Drawing primitives
   ───────────────────────────────────────────────────────────────────────── */

void ILI9488_FillScreen(uint16_t colour)
{
    ILI9488_FillRect(0, 0, _width, _height, colour);
}

void ILI9488_DrawPixel(uint16_t x, uint16_t y, uint16_t colour)
{
    if (x >= _width || y >= _height) return;
    ILI9488_SetWindow(x, y, x, y);
    uint8_t pix[3] = { RGB565_TO_R6(colour), RGB565_TO_G6(colour), RGB565_TO_B6(colour) };
    HAL_SPI_Transmit(_hspi, pix, 3, HAL_MAX_DELAY);
    ILI9488_CS_Deassert();
}

void ILI9488_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour)
{
    if (x >= _width || y >= _height) return;
    if (x + w > _width)  w = _width  - x;
    if (y + h > _height) h = _height - y;

    uint8_t r = RGB565_TO_R6(colour);
    uint8_t g = RGB565_TO_G6(colour);
    uint8_t b = RGB565_TO_B6(colour);

    #define FILL_BATCH 128u
    uint8_t buf[FILL_BATCH * 3u];
    for (uint32_t i = 0; i < FILL_BATCH; i++) {
        buf[i*3+0] = r; buf[i*3+1] = g; buf[i*3+2] = b;
    }

    ILI9488_SetWindow(x, y, (uint16_t)(x+w-1u), (uint16_t)(y+h-1u));

    uint32_t total = (uint32_t)w * h;
    while (total > 0u) {
        uint32_t chunk = (total > FILL_BATCH) ? FILL_BATCH : total;
        HAL_SPI_Transmit(_hspi, buf, (uint16_t)(chunk * 3u), HAL_MAX_DELAY);
        total -= chunk;
    }
    ILI9488_CS_Deassert();
    #undef FILL_BATCH
}

void ILI9488_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t colour)
{
    ILI9488_FillRect(x, y, len, 1u, colour);
}

void ILI9488_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t colour)
{
    ILI9488_FillRect(x, y, 1u, len, colour);
}

void ILI9488_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour)
{
    ILI9488_DrawHLine(x,          y,          w,    colour);
    ILI9488_DrawHLine(x,          y+h-1u,     w,    colour);
    ILI9488_DrawVLine(x,          y+1u,       h-2u, colour);
    ILI9488_DrawVLine(x+w-1u,     y+1u,       h-2u, colour);
}

void ILI9488_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t colour)
{
    int16_t dx  =  (int16_t)abs((int16_t)x1 - (int16_t)x0);
    int16_t dy  = -(int16_t)abs((int16_t)y1 - (int16_t)y0);
    int16_t sx  = (x0 < x1) ?  1 : -1;
    int16_t sy  = (y0 < y1) ?  1 : -1;
    int16_t err = dx + dy;

    while (1) {
        ILI9488_DrawPixel(x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 = (uint16_t)((int16_t)x0 + sx); }
        if (e2 <= dx) { err += dx; y0 = (uint16_t)((int16_t)y0 + sy); }
    }
}

static void _DrawCirclePoints(uint16_t cx, uint16_t cy, int16_t x, int16_t y, uint16_t colour)
{
    ILI9488_DrawPixel((uint16_t)(cx+x), (uint16_t)(cy+y), colour);
    ILI9488_DrawPixel((uint16_t)(cx-x), (uint16_t)(cy+y), colour);
    ILI9488_DrawPixel((uint16_t)(cx+x), (uint16_t)(cy-y), colour);
    ILI9488_DrawPixel((uint16_t)(cx-x), (uint16_t)(cy-y), colour);
    ILI9488_DrawPixel((uint16_t)(cx+y), (uint16_t)(cy+x), colour);
    ILI9488_DrawPixel((uint16_t)(cx-y), (uint16_t)(cy+x), colour);
    ILI9488_DrawPixel((uint16_t)(cx+y), (uint16_t)(cy-x), colour);
    ILI9488_DrawPixel((uint16_t)(cx-y), (uint16_t)(cy-x), colour);
}

void ILI9488_DrawCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t colour)
{
    int16_t x = 0, y = (int16_t)r;
    int16_t d = 3 - 2*(int16_t)r;
    while (y >= x) {
        _DrawCirclePoints(cx, cy, x, y, colour);
        x++;
        if (d < 0) { d += 4*x + 6; }
        else       { d += 4*(x - y) + 10; y--; }
    }
}

void ILI9488_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t colour)
{
    for (int16_t y = -(int16_t)r; y <= (int16_t)r; y++) {
        int16_t dx = (int16_t)_isqrt((uint32_t)((int32_t)r*r - (int32_t)y*y));
        ILI9488_DrawHLine((uint16_t)(cx - dx), (uint16_t)(cy + y), (uint16_t)(2*dx+1), colour);
    }
}

/* ─────────────────────────────────────────────────────────────────────────
   Built-in 5x7 font (ASCII 32-122)
   ───────────────────────────────────────────────────────────────────────── */
static const uint8_t _font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* '\''*/
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x14,0x08,0x3E,0x08,0x14}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\'*/
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
};

void ILI9488_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t size)
{
    if (c < 32 || c > 122) c = '?';
    const uint8_t *bitmap = _font5x7[c - 32];
    uint8_t r_fg = RGB565_TO_R6(fg), g_fg = RGB565_TO_G6(fg), b_fg = RGB565_TO_B6(fg);
    uint8_t r_bg = RGB565_TO_R6(bg), g_bg = RGB565_TO_G6(bg), b_bg = RGB565_TO_B6(bg);

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t colData = bitmap[col];
        for (uint8_t row = 0; row < 7; row++) {
            uint16_t px = x + col * size;
            uint16_t py = y + row * size;
            bool set = (colData >> row) & 0x01u;
            uint8_t r = set ? r_fg : r_bg;
            uint8_t g = set ? g_fg : g_bg;
            uint8_t b = set ? b_fg : b_bg;
            if (size == 1) {
                ILI9488_SetWindow(px, py, px, py);
                uint8_t pix[3] = {r, g, b};
                HAL_SPI_Transmit(_hspi, pix, 3, HAL_MAX_DELAY);
                ILI9488_CS_Deassert();
            } else {
                ILI9488_FillRect(px, py, size, size, set ? fg : bg);
            }
        }
    }
}

void ILI9488_DrawString(uint16_t x, uint16_t y, const char *str,
                         uint16_t fg, uint16_t bg, uint8_t size)
{
    uint16_t cursor_x = x;
    while (*str) {
        ILI9488_DrawChar(cursor_x, y, *str, fg, bg, size);
        cursor_x += (uint16_t)(6u * size);
        str++;
    }
}

/* ─────────────────────────────────────────────────────────────────────────
   TouchGFX HAL Integration hook
   ───────────────────────────────────────────────────────────────────────── */
void ILI9488_CopyFrameBufferBlockToLCD(const uint16_t *pSrc,
                                        uint16_t x, uint16_t y,
                                        uint16_t width, uint16_t height)
{
    if (!pSrc || width == 0u || height == 0u) return;
    ILI9488_SetWindow(x, y, (uint16_t)(x+width-1u), (uint16_t)(y+height-1u));
    ILI9488_WritePixels_RGB565(pSrc, (uint32_t)width * height);
}
