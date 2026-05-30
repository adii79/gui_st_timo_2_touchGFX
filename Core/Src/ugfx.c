/**
 ******************************************************************************
 * @file    ugfx.c
 * @brief   µGFX implementation — declarative widget engine
 *
 * Changes from previous version:
 *   • ugfx_action_cb now receives (ugfx_button_t *btn) so the callback can
 *     mutate the button (label, colors, …) before the engine redraws it.
 *   • on_tap fires on PRESS (finger-down), not on release, for instant feel.
 *   • Engine redraws the button AFTER the callback returns so mutations show.
 *   • Drag-off cancels the press without re-firing the callback.
 *   • Release just restores the idle visual — no double-fire.
 ******************************************************************************
 */

#include "ugfx.h"

/* ══════════════════════════════════════════════════════════════════════════
   STATIC POOLS  (no heap — fixed-size arrays)
   ══════════════════════════════════════════════════════════════════════════ */

static ugfx_slider_t _sliders[UGFX_MAX_SLIDERS];
static ugfx_button_t _buttons[UGFX_MAX_BUTTONS];
static ugfx_icon_t   _icons  [UGFX_MAX_ICONS];
static ugfx_label_t  _labels [UGFX_MAX_LABELS];

static uint8_t _n_sliders, _n_buttons, _n_icons, _n_labels;

/* Active builder scratch space (one per type — builders are short-lived) */
static ugfx_slider_builder_t _sb;
static ugfx_button_builder_t _bb;
static ugfx_icon_builder_t   _ib;
static ugfx_label_builder_t  _lbb;

/* Global state */
static ugfx_rot_t _rotation    = UGFX_ROT_0;
static bool       _was_touched = false;

/* ══════════════════════════════════════════════════════════════════════════
   COORDINATE TRANSFORM  (rotation)
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  Transform a touch point (raw pixel from XPT2046) into the
 *         rotated coordinate space used by widget origins.
 */
static void _TransformTouch(uint16_t px, uint16_t py,
                             uint16_t *lx, uint16_t *ly)
{
    uint16_t W = ILI9488_WIDTH;
    uint16_t H = ILI9488_HEIGHT;

    switch (_rotation) {
        default:
        case UGFX_ROT_0:
            *lx = px;              *ly = py;              break;
        case UGFX_ROT_90:
            *lx = py;              *ly = W - 1u - px;     break;
        case UGFX_ROT_180:
            *lx = W - 1u - px;    *ly = H - 1u - py;     break;
        case UGFX_ROT_270:
            *lx = H - 1u - py;    *ly = px;              break;
    }
}

/**
 * @brief  Transform a logical widget origin → physical display pixel.
 *         Used when drawing so the widget appears in the expected location.
 */
static void _TransformOrigin(uint16_t lx, uint16_t ly,
                              uint16_t *px, uint16_t *py)
{
    uint16_t W = ILI9488_WIDTH;
    uint16_t H = ILI9488_HEIGHT;

    switch (_rotation) {
        default:
        case UGFX_ROT_0:
            *px = lx;              *py = ly;              break;
        case UGFX_ROT_90:
            *px = W - 1u - ly;    *py = lx;              break;
        case UGFX_ROT_180:
            *px = W - 1u - lx;    *py = H - 1u - ly;    break;
        case UGFX_ROT_270:
            *px = ly;              *py = H - 1u - lx;    break;
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   SLIDER DRAWING
   ══════════════════════════════════════════════════════════════════════════ */

static uint16_t _SliderValToPos(const ugfx_slider_t *s)
{
    int32_t range = s->val_max - s->val_min;
    if (range == 0) return 0u;

    uint16_t length = (s->dir == UGFX_HORIZONTAL) ? s->w : s->h;
    int32_t  travel = (int32_t)(length - 2u * UGFX_KNOB_R);

    int32_t v = s->value - s->val_min;
    if (v < 0)     v = 0;
    if (v > range) v = range;

    int32_t pos = (v * travel) / range + (int32_t)UGFX_KNOB_R;

    /* Vertical: invert so high value = knob at TOP, low value = knob at BOTTOM */
    if (s->dir == UGFX_VERTICAL) {
        pos = (int32_t)length - pos - (int32_t)UGFX_KNOB_R;
        if (pos < (int32_t)UGFX_KNOB_R)            pos = (int32_t)UGFX_KNOB_R;
        if (pos > (int32_t)(length - UGFX_KNOB_R)) pos = (int32_t)(length - UGFX_KNOB_R);
    }

    return (uint16_t)pos;
}

static int32_t _SliderPosToVal(const ugfx_slider_t *s, int32_t pos)
{
    uint16_t length = (s->dir == UGFX_HORIZONTAL) ? s->w : s->h;
    int32_t  travel = (int32_t)(length - 2u * UGFX_KNOB_R);

    /* Vertical: invert touch position so dragging UP increases value */
    if (s->dir == UGFX_VERTICAL) {
        pos = (int32_t)length - pos - (int32_t)UGFX_KNOB_R;
    }

    int32_t offset = pos - (int32_t)UGFX_KNOB_R;
    if (offset < 0)      offset = 0;
    if (offset > travel) offset = travel;

    int32_t range = s->val_max - s->val_min;
    return s->val_min + (offset * range) / travel;
}

void UGFX_SliderDraw(ugfx_slider_t *s)
{
    if (!s || !s->_active) return;

    uint16_t px, py;
    _TransformOrigin(s->x, s->y, &px, &py);

    /* Erase bounding box */
//    ILI9488_FillRect(px, py, s->w, s->h, UGFX_COL_BG);
    /* NEW — pad by UGFX_KNOB_R on every side */
    uint16_t erase_x = (px >= UGFX_KNOB_R) ? px - UGFX_KNOB_R : 0u;
    uint16_t erase_y = (py >= UGFX_KNOB_R) ? py - UGFX_KNOB_R : 0u;
    uint16_t erase_w = s->w + 2u * UGFX_KNOB_R;
    uint16_t erase_h = s->h + 2u * UGFX_KNOB_R;
    ILI9488_FillRect(erase_x, erase_y, erase_w, erase_h, UGFX_COL_BG);

    uint16_t knob_pos = _SliderValToPos(s);

    if (s->dir == UGFX_HORIZONTAL) {
        uint16_t track_y = py + s->h / 2u - UGFX_TRACK_H / 2u;

        ILI9488_FillRect(px + UGFX_KNOB_R, track_y,
                         s->w - 2u * UGFX_KNOB_R, UGFX_TRACK_H,
                         s->col_track);

        if (knob_pos > UGFX_KNOB_R) {
            ILI9488_FillRect(px + UGFX_KNOB_R, track_y,
                             knob_pos - UGFX_KNOB_R, UGFX_TRACK_H,
                             s->col_fill);
        }

        uint16_t kx = px + knob_pos;
        uint16_t ky = py + s->h / 2u;
        ILI9488_FillCircle(kx, ky, UGFX_KNOB_R,        s->col_knob);
        ILI9488_FillCircle(kx - 3u, ky - 3u, 3u,       0xFFFFu);

    } else {
        /* VERTICAL slider — knob at TOP when value is HIGH */
        uint16_t track_x = px + s->w / 2u - UGFX_TRACK_H / 2u;
        uint16_t track_top    = py + UGFX_KNOB_R;
        uint16_t track_len    = s->h - 2u * UGFX_KNOB_R;

        /* Full track (dark) */
        ILI9488_FillRect(track_x, track_top, UGFX_TRACK_H, track_len, s->col_track);

        /* Fill: from knob downward to bottom of track (value represented below knob) */
        uint16_t fill_top = py + knob_pos;
        if (fill_top < py + s->h - UGFX_KNOB_R) {
            ILI9488_FillRect(track_x,
                             fill_top,
                             UGFX_TRACK_H,
                             (py + s->h - UGFX_KNOB_R) - fill_top,
                             s->col_fill);
        }

        /* Knob */
        uint16_t kx = px + s->w / 2u;
        uint16_t ky = py + knob_pos;
        ILI9488_FillCircle(kx, ky, UGFX_KNOB_R,   s->col_knob);
        ILI9488_FillCircle(kx - 3u, ky - 3u, 3u,  0xFFFFu);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   BUTTON DRAWING
   ══════════════════════════════════════════════════════════════════════════ */

void UGFX_ButtonDraw(ugfx_button_t *b, bool pressed)
{
    if (!b || !b->_active) return;

    uint16_t px, py;
    _TransformOrigin(b->x, b->y, &px, &py);

    uint16_t face   = pressed ? b->col_press  : b->col_idle;
    uint16_t border = pressed ? 0x07FFu       : b->col_border;

    ILI9488_FillRect(px, py, b->w, b->h, face);
    ILI9488_DrawRect(px, py, b->w, b->h, border);

    if (!pressed) {
        ILI9488_DrawRect((uint16_t)(px + 1u), (uint16_t)(py + 1u),
                         (uint16_t)(b->w - 2u), (uint16_t)(b->h - 2u),
                         0x39E7u);   /* subtle inner bevel */
    }

    if (b->label) {
        uint8_t  sz = b->label_size;
        uint16_t tw = (uint16_t)(strlen(b->label) * 6u * sz);
        uint16_t th = (uint16_t)(7u * sz);
        uint16_t lx = (uint16_t)(px + (b->w > tw ? (b->w - tw) / 2u : 2u));
        uint16_t ly = (uint16_t)(py + (b->h > th ? (b->h - th) / 2u : 2u));
        ILI9488_DrawString(lx, ly, b->label, b->col_text, face, sz);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
   ICON DRAWING
   ══════════════════════════════════════════════════════════════════════════ */

void UGFX_IconDraw(ugfx_icon_t *ic)
{
    if (!ic || !ic->_active || !ic->bmp) return;
    uint16_t px, py;
    _TransformOrigin(ic->x, ic->y, &px, &py);
    UI_DrawBitmap(px, py, ic->bmp, ic->scale, ic->color);
}

/* ══════════════════════════════════════════════════════════════════════════
   LABEL DRAWING
   ══════════════════════════════════════════════════════════════════════════ */

//void UGFX_LabelDraw(ugfx_label_t *lbl)
//{
//    if (!lbl || !lbl->_active) return;
//    uint16_t px, py;
//    _TransformOrigin(lbl->x, lbl->y, &px, &py);
//    ILI9488_DrawString(px, py, lbl->text, lbl->col_fg, lbl->col_bg, lbl->size);
//    lbl->_dirty = false;
//}

void UGFX_LabelDraw(ugfx_label_t *lbl)
{
    if (!lbl || !lbl->_active) return;
    uint16_t px, py;
    _TransformOrigin(lbl->x, lbl->y, &px, &py);

    /* Erase old text area first (max possible label width) */
    uint16_t erase_w = (uint16_t)(sizeof(lbl->text) * 6u * lbl->size);
    uint16_t erase_h = (uint16_t)(7u * lbl->size);
    ILI9488_FillRect(px, py, erase_w, erase_h, lbl->col_bg);

    ILI9488_DrawString(px, py, lbl->text, lbl->col_fg, lbl->col_bg, lbl->size);
    lbl->_dirty = false;
}


/* ══════════════════════════════════════════════════════════════════════════
   TOUCH HIT TESTS
   ══════════════════════════════════════════════════════════════════════════ */

static bool _InRect(uint16_t tx, uint16_t ty,
                    uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh)
{
    return (tx >= rx && tx < rx + rw && ty >= ry && ty < ry + rh);
}

/* ══════════════════════════════════════════════════════════════════════════
   SLIDER TOUCH HANDLER
   ══════════════════════════════════════════════════════════════════════════ */

static bool _SliderTouch(ugfx_slider_t *s, uint16_t lx, uint16_t ly)
{
    if (!s->_active)                    return false;
    if (!(s->touch & UGFX_TOUCH_DRAG)) return false;

    /* Extended hit area: knob radius padding on every side */
    uint16_t hx = (s->x > UGFX_KNOB_R) ? s->x - UGFX_KNOB_R : 0u;
    uint16_t hy = (s->y > UGFX_KNOB_R) ? s->y - UGFX_KNOB_R : 0u;
    uint16_t hw = s->w + 2u * UGFX_KNOB_R;
    uint16_t hh = s->h + 2u * UGFX_KNOB_R;

    if (!_InRect(lx, ly, hx, hy, hw, hh)) return false;

    int32_t new_val;
    if (s->dir == UGFX_HORIZONTAL) {
        new_val = _SliderPosToVal(s, (int32_t)lx - (int32_t)s->x);
    } else {
        new_val = _SliderPosToVal(s, (int32_t)ly - (int32_t)s->y);
    }

    if (new_val != s->value) {
        s->value = new_val;
        UGFX_SliderDraw(s);
        if (s->on_changed) s->on_changed(s->value);
    }
    s->_dragging = true;
    return true;
}

static void _SliderRelease(ugfx_slider_t *s)
{
    s->_dragging = false;
}

/* ══════════════════════════════════════════════════════════════════════════
   BUTTON HIT TEST
   ══════════════════════════════════════════════════════════════════════════ */

static bool _ButtonInBounds(const ugfx_button_t *b, uint16_t lx, uint16_t ly)
{
    return _InRect(lx, ly, b->x, b->y, b->w, b->h);
}

/* ══════════════════════════════════════════════════════════════════════════
   PUBLIC API IMPLEMENTATION
   ══════════════════════════════════════════════════════════════════════════ */

void UGFX_Init(void)
{
    memset(_sliders, 0, sizeof(_sliders));
    memset(_buttons, 0, sizeof(_buttons));
    memset(_icons,   0, sizeof(_icons));
    memset(_labels,  0, sizeof(_labels));
    _n_sliders = _n_buttons = _n_icons = _n_labels = 0u;
    _rotation    = UGFX_ROT_0;
    _was_touched = false;
}

void UGFX_Begin(void)
{
    for (uint8_t i = 0; i < UGFX_MAX_SLIDERS; i++) _sliders[i]._active = false;
    for (uint8_t i = 0; i < UGFX_MAX_BUTTONS; i++) _buttons[i]._active = false;
    for (uint8_t i = 0; i < UGFX_MAX_ICONS;   i++) _icons  [i]._active = false;
    for (uint8_t i = 0; i < UGFX_MAX_LABELS;  i++) _labels [i]._active = false;
    _n_sliders = _n_buttons = _n_icons = _n_labels = 0u;
}

void UGFX_Commit(void)
{
    for (uint8_t i = 0; i < _n_icons;   i++) UGFX_IconDraw  (&_icons  [i]);
    for (uint8_t i = 0; i < _n_buttons; i++) UGFX_ButtonDraw(&_buttons[i], false);
    for (uint8_t i = 0; i < _n_sliders; i++) UGFX_SliderDraw(&_sliders[i]);
    for (uint8_t i = 0; i < _n_labels;  i++) UGFX_LabelDraw (&_labels [i]);
}

void UGFX_SetRotation(ugfx_rot_t rot)
{
    _rotation = rot;
}

/* ── Runtime helpers ────────────────────────────────────────────────────── */

void UGFX_SliderSetValue(ugfx_slider_t *s, int32_t value)
{
    if (!s) return;
    if (value < s->val_min) value = s->val_min;
    if (value > s->val_max) value = s->val_max;
    s->value = value;
    UGFX_SliderDraw(s);
}

int32_t UGFX_SliderGetValue(const ugfx_slider_t *s)
{
    return s ? s->value : 0;
}

void UGFX_LabelSetText(ugfx_label_t *lbl, const char *text)
{
    if (!lbl || !text) return;
    strncpy(lbl->text, text, sizeof(lbl->text) - 1u);
    lbl->text[sizeof(lbl->text) - 1u] = '\0';
    lbl->_dirty = true;
    UGFX_LabelDraw(lbl);
}

/* ══════════════════════════════════════════════════════════════════════════
   POLL  — main event loop, call from while(1)
   ══════════════════════════════════════════════════════════════════════════ */

void UGFX_Poll(void)
{
    XPT2046_Point_t pt;
    bool touching = XPT2046_GetTouchPixel(&pt) ? true : false;

    uint16_t lx = 0u, ly = 0u;
    if (touching) {
        _TransformTouch(pt.x, pt.y, &lx, &ly);
    }

    /* ══ FINGER DOWN / DRAGGING ══════════════════════════════════════════ */
    if (touching) {

        /* ── Sliders (DRAG) ───────────────────────────────────────────── */
        for (uint8_t i = 0; i < _n_sliders; i++) {
            _SliderTouch(&_sliders[i], lx, ly);
        }

        /* ── Buttons (TAP — fires on press, redraws after callback) ───── */
        for (uint8_t i = 0; i < _n_buttons; i++) {
            ugfx_button_t *b = &_buttons[i];

            if (!b->_active)                   continue;
            if (!(b->touch & UGFX_TOUCH_TAP)) continue;

            bool in = _ButtonInBounds(b, lx, ly);

            if (in && !b->_pressed) {
                /* ── Finger just landed inside ── */
                b->_pressed = true;

                /* 1. Draw pressed state so user sees instant feedback */
                UGFX_ButtonDraw(b, true);

                /* 2. Call the user callback — it may mutate b->label,
                      b->col_idle, b->col_press, b->col_text, etc.      */
                if (b->on_tap) {
                    b->on_tap(b);
                }

                /* 3. Redraw to show any mutations the callback made
                      (still in pressed state while finger is down)      */
                UGFX_ButtonDraw(b, true);

            } else if (!in && b->_pressed) {
                /* ── Finger dragged off — cancel, no callback ── */
                b->_pressed = false;
                UGFX_ButtonDraw(b, false);
            }
        }

    /* ══ FINGER LIFTED ═══════════════════════════════════════════════════ */
    } else {
        if (_was_touched) {

            /* Release all sliders */
            for (uint8_t i = 0; i < _n_sliders; i++) {
                _SliderRelease(&_sliders[i]);
            }

            /* Restore button idle visuals (callback already fired on press) */
            for (uint8_t i = 0; i < _n_buttons; i++) {
                ugfx_button_t *b = &_buttons[i];
                if (b->_pressed) {
                    b->_pressed = false;
                    UGFX_ButtonDraw(b, false);
                    /* NOTE: on_tap already fired on finger-down above.
                             Do NOT call it again here.                  */
                }
            }
        }
    }

    _was_touched = touching;
}

/* ══════════════════════════════════════════════════════════════════════════
   BUILDER IMPLEMENTATIONS
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Slider builder ─────────────────────────────────────────────────────── */

static ugfx_slider_builder_t *_SB_frame(ugfx_slider_builder_t *b,
                                         uint16_t w, uint16_t h)
    { b->cfg.w = w; b->cfg.h = h; return b; }

static ugfx_slider_builder_t *_SB_origin(ugfx_slider_builder_t *b,
                                           uint16_t x, uint16_t y)
    { b->cfg.x = x; b->cfg.y = y; return b; }

static ugfx_slider_builder_t *_SB_direction(ugfx_slider_builder_t *b,
                                              ugfx_dir_t d)
    { b->cfg.dir = d; return b; }

static ugfx_slider_builder_t *_SB_colors(ugfx_slider_builder_t *b,
                                           uint16_t track,
                                           uint16_t fill,
                                           uint16_t knob)
    { b->cfg.col_track = track;
      b->cfg.col_fill  = fill;
      b->cfg.col_knob  = knob;
      return b; }

static ugfx_slider_builder_t *_SB_touchMask(ugfx_slider_builder_t *b,
                                              ugfx_touch_mask_t m)
    { b->cfg.touch = m; return b; }

static ugfx_slider_builder_t *_SB_onChanged(ugfx_slider_builder_t *b,
                                              ugfx_value_cb cb)
    { b->cfg.on_changed = cb; return b; }

static ugfx_slider_t *_SB_build(ugfx_slider_builder_t *b)
{
    if (_n_sliders >= UGFX_MAX_SLIDERS) return NULL;
    ugfx_slider_t *s = &_sliders[_n_sliders++];
    *s           = b->cfg;
    s->_active   = true;
    s->_dragging = false;
    return s;
}

ugfx_slider_builder_t *Slider(int32_t min, int32_t max, int32_t initial)
{
    memset(&_sb, 0, sizeof(_sb));
    _sb.cfg.val_min   = min;
    _sb.cfg.val_max   = max;
    _sb.cfg.value     = initial;
    _sb.cfg.col_track = UGFX_COL_TRACK;
    _sb.cfg.col_fill  = UGFX_COL_FILL;
    _sb.cfg.col_knob  = UGFX_COL_KNOB;
    _sb.cfg.touch     = UGFX_TOUCH_DRAG;
    _sb.cfg.h         = (uint16_t)(UGFX_KNOB_R * 2u + 2u);

    _sb.frame      = _SB_frame;
    _sb.origin     = _SB_origin;
    _sb.direction  = _SB_direction;
    _sb.colors     = _SB_colors;
    _sb.touchMask  = _SB_touchMask;
    _sb.onChanged  = _SB_onChanged;
    _sb.build      = _SB_build;
    return &_sb;
}

/* ── Button builder ─────────────────────────────────────────────────────── */

static ugfx_button_builder_t *_BB_frame(ugfx_button_builder_t *b,
                                          uint16_t w, uint16_t h)
    { b->cfg.w = w; b->cfg.h = h; return b; }

static ugfx_button_builder_t *_BB_origin(ugfx_button_builder_t *b,
                                           uint16_t x, uint16_t y)
    { b->cfg.x = x; b->cfg.y = y; return b; }

static ugfx_button_builder_t *_BB_labelSize(ugfx_button_builder_t *b,
                                              uint8_t s)
    { b->cfg.label_size = s; return b; }

static ugfx_button_builder_t *_BB_colors(ugfx_button_builder_t *b,
                                           uint16_t idle, uint16_t press,
                                           uint16_t border, uint16_t text)
    { b->cfg.col_idle   = idle;
      b->cfg.col_press  = press;
      b->cfg.col_border = border;
      b->cfg.col_text   = text;
      return b; }

static ugfx_button_builder_t *_BB_touchMask(ugfx_button_builder_t *b,
                                              ugfx_touch_mask_t m)
    { b->cfg.touch = m; return b; }

static ugfx_button_builder_t *_BB_onTap(ugfx_button_builder_t *b,
                                          ugfx_action_cb cb)
    { b->cfg.on_tap = cb; return b; }

static ugfx_button_t *_BB_build(ugfx_button_builder_t *b)
{
    if (_n_buttons >= UGFX_MAX_BUTTONS) return NULL;
    ugfx_button_t *btn = &_buttons[_n_buttons++];
    *btn           = b->cfg;
    btn->_active   = true;
    btn->_pressed  = false;
    return btn;
}

ugfx_button_builder_t *Button(const char *label)
{
    memset(&_bb, 0, sizeof(_bb));
    _bb.cfg.label      = label;
    _bb.cfg.label_size = 2u;
    _bb.cfg.col_idle   = UGFX_COL_BTN_IDLE;
    _bb.cfg.col_press  = UGFX_COL_BTN_PRESS;
    _bb.cfg.col_border = UGFX_COL_BTN_BORDER;
    _bb.cfg.col_text   = UGFX_COL_BTN_TXT;
    _bb.cfg.touch      = UGFX_TOUCH_TAP;

    _bb.frame      = _BB_frame;
    _bb.origin     = _BB_origin;
    _bb.labelSize  = _BB_labelSize;
    _bb.colors     = _BB_colors;
    _bb.touchMask  = _BB_touchMask;
    _bb.onTap      = _BB_onTap;
    _bb.build      = _BB_build;
    return &_bb;
}

/* ── Icon builder ───────────────────────────────────────────────────────── */

static ugfx_icon_builder_t *_IB_origin(ugfx_icon_builder_t *b,
                                         uint16_t x, uint16_t y)
    { b->cfg.x = x; b->cfg.y = y; return b; }

static ugfx_icon_builder_t *_IB_scale(ugfx_icon_builder_t *b, uint8_t s)
    { b->cfg.scale = s; return b; }

static ugfx_icon_builder_t *_IB_color(ugfx_icon_builder_t *b, uint16_t c)
    { b->cfg.color = c; return b; }

static ugfx_icon_t *_IB_build(ugfx_icon_builder_t *b)
{
    if (_n_icons >= UGFX_MAX_ICONS) return NULL;
    ugfx_icon_t *ic = &_icons[_n_icons++];
    *ic          = b->cfg;
    ic->_active  = true;
    return ic;
}

ugfx_icon_builder_t *Icon(const UI_Bitmap_t *bmp)
{
    memset(&_ib, 0, sizeof(_ib));
    _ib.cfg.bmp   = bmp;
    _ib.cfg.scale = 2u;
    _ib.cfg.color = 0xFFFFu;

    _ib.origin = _IB_origin;
    _ib.scale  = _IB_scale;
    _ib.color  = _IB_color;
    _ib.build  = _IB_build;
    return &_ib;
}

/* ── Label builder ──────────────────────────────────────────────────────── */

static ugfx_label_builder_t *_LB_origin(ugfx_label_builder_t *b,
                                          uint16_t x, uint16_t y)
    { b->cfg.x = x; b->cfg.y = y; return b; }

static ugfx_label_builder_t *_LB_color(ugfx_label_builder_t *b,
                                         uint16_t fg, uint16_t bg)
    { b->cfg.col_fg = fg; b->cfg.col_bg = bg; return b; }

static ugfx_label_builder_t *_LB_size(ugfx_label_builder_t *b, uint8_t s)
    { b->cfg.size = s; return b; }

static ugfx_label_t *_LB_build(ugfx_label_builder_t *b)
{
    if (_n_labels >= UGFX_MAX_LABELS) return NULL;
    ugfx_label_t *lbl = &_labels[_n_labels++];
    *lbl          = b->cfg;
    lbl->_active  = true;
    lbl->_dirty   = false;
    return lbl;
}

ugfx_label_builder_t *Label(const char *text)
{
    memset(&_lbb, 0, sizeof(_lbb));
    strncpy(_lbb.cfg.text, text, sizeof(_lbb.cfg.text) - 1u);
    _lbb.cfg.col_fg = UGFX_COL_LABEL;
    _lbb.cfg.col_bg = UGFX_COL_BG;
    _lbb.cfg.size   = 1u;

    _lbb.origin = _LB_origin;
    _lbb.color  = _LB_color;
    _lbb.size   = _LB_size;
    _lbb.build  = _LB_build;
    return &_lbb;
}
