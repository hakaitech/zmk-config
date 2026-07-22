/*
 * Custom Corne status screens (SSD1306, mounted 90deg CCW; art in screen_art.h is
 * pre-rotated to native 128x32).
 *
 * Both halves show a keypress PULSE (expanding-ring "ping") in the top zone, fired
 * on every key press. Each half raises zmk_position_state_changed LOCALLY for its
 * own keys, so we filter to source==LOCAL and each display pulses on its own keys.
 *
 * Left (central) - "Float": layer initial in a box + charging + battery, plus the
 *   pulse at the top.
 * Right (peripheral) - functional: pulse (top) / battery % as a big number
 *   (center) / Bluetooth link status (bottom).
 *
 * The event listener only sets a flag; all LVGL work happens on the display timer
 * (LVGL thread), so no cross-thread LVGL calls. The pulse advances one frame per
 * tick (~50ms) and retriggers from frame 0 on a fresh press.
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <lvgl.h>

#include "screen_art.h"

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/keymap.h>
#include <zmk/usb.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#endif

#define TICK_MS 50

/* native coords: arg2 = native_x (viewer vertical), arg3 = native_y (viewer horizontal) */
#define PULSE_NX 107      /* pulse top zone, centered on viewer_x=16 (viewer top-left ~6,1) */
#define PULSE_NY 6
#define PULSE_FRAMES 5

/* ── shared helpers ── */
static void set_hidden(lv_obj_t *obj, bool hidden) {
    if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

/* keypress → flag (event thread); the pulse is advanced on the LVGL timer */
static volatile bool kp_hit = false;
static int kp_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev && ev->state && ev->source == ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL) {
        kp_hit = true;
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(kp_disp, kp_listener);
ZMK_SUBSCRIPTION(kp_disp, zmk_position_state_changed);

/* pulse: expanding-ring frames, cycled once per press (used by both halves) */
static const lv_img_dsc_t *const pulse_dsc[PULSE_FRAMES] = {
    &pulse_0, &pulse_1, &pulse_2, &pulse_3, &pulse_4,
};
static lv_obj_t *pulse_img;
static int pulse_i = -1;   /* -1 idle; 0..PULSE_FRAMES-1 playing */
static void render_pulse(void) {
    if (kp_hit) { kp_hit = false; pulse_i = 0; }
    if (pulse_i < 0) { set_hidden(pulse_img, true); return; }
    lv_image_set_src(pulse_img, pulse_dsc[pulse_i]);
    set_hidden(pulse_img, false);
    if (++pulse_i >= PULSE_FRAMES) pulse_i = -1;
}
static void make_pulse(lv_obj_t *screen) {
    pulse_img = lv_img_create(screen);
    lv_obj_set_pos(pulse_img, PULSE_NX, PULSE_NY);
    pulse_i = -1;
}

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

/* ── Left · Float + pulse ── */
static const lv_img_dsc_t *const digit_dsc[10] = {
    &dig_0, &dig_1, &dig_2, &dig_3, &dig_4, &dig_5, &dig_6, &dig_7, &dig_8, &dig_9,
};
static lv_obj_t *ltr_img, *bolt_img, *plug_img, *fill_rect;
static lv_obj_t *pct_chars[4];

static lv_obj_t *make_fill(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_color(o, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

/* small percentage row: tiny glyphs, centered on center_vx */
static void set_pct_row(uint8_t pct, int native_x, int center_vx) {
    char buf[5];
    snprintf(buf, sizeof(buf), "%u%%", pct > 100 ? 100 : pct);
    int len = strlen(buf);
    int vx = center_vx - (len * 4 - 1) / 2;
    for (int i = 0; i < 4; i++) {
        if (i < len) {
            lv_image_set_src(pct_chars[i], buf[i] == '%' ? &dig_pct : digit_dsc[buf[i] - '0']);
            lv_obj_set_pos(pct_chars[i], native_x, vx + i * 4);
            set_hidden(pct_chars[i], false);
        } else {
            set_hidden(pct_chars[i], true);
        }
    }
}

static void tick_cb(lv_timer_t *timer) {
    render_pulse();

    static const lv_img_dsc_t *const letters[] = {&ltr_b, &ltr_s, &ltr_n};
    zmk_keymap_layer_index_t li = zmk_keymap_highest_layer_active();
    lv_image_set_src(ltr_img, letters[li < ARRAY_SIZE(letters) ? li : 0]);

    enum zmk_usb_conn_state usb = zmk_usb_get_conn_state();
    set_hidden(bolt_img, usb == ZMK_USB_CONN_NONE);
    set_hidden(plug_img, usb != ZMK_USB_CONN_HID);

    uint8_t pct = zmk_battery_state_of_charge();
    set_pct_row(pct, LPCT_NX, 21);
    int fw = LBAR_MAX * pct / 100;
    set_hidden(fill_rect, fw == 0);
    if (fw > 0) lv_obj_set_size(fill_rect, 3, fw);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *bg = lv_img_create(screen);
    lv_image_set_src(bg, &bg_left);
    lv_obj_set_pos(bg, 0, 0);

    ltr_img = lv_img_create(screen);
    lv_obj_set_pos(ltr_img, LTR_X, LTR_Y);
    bolt_img = lv_img_create(screen);
    lv_image_set_src(bolt_img, &ico_bolt);
    lv_obj_set_pos(bolt_img, ICO_BOLT_X, ICO_BOLT_Y);
    plug_img = lv_img_create(screen);
    lv_image_set_src(plug_img, &ico_plug);
    lv_obj_set_pos(plug_img, ICO_PLUG_X, ICO_PLUG_Y);

    fill_rect = make_fill(screen);
    lv_obj_set_pos(fill_rect, LBAR_NX, LBAR_NY);
    for (int i = 0; i < 4; i++) pct_chars[i] = lv_img_create(screen);
    make_pulse(screen);

    lv_timer_create(tick_cb, TICK_MS, NULL);
    tick_cb(NULL);
    return screen;
}

#else /* peripheral */

/* ── Right · pulse / battery% / link ── */
#define BATT_NX    72     /* big battery digits (viewer_y ~42) */
#define BATT_PITCH 11
#define BPCT_NX    79     /* small % after the big number */
#define BPCT_W     4
#define LINK_NX    25     /* link rune, bottom (viewer_y ~92) */
#define LINK_NY    11

static const lv_img_dsc_t *const big_digit[10] = {
    &bd_0, &bd_1, &bd_2, &bd_3, &bd_4, &bd_5, &bd_6, &bd_7, &bd_8, &bd_9,
};
static lv_obj_t *rune_img;
static lv_obj_t *batt_chars[3];
static lv_obj_t *batt_pct_obj;
static int batt_shown = -1;

static void render_batt(uint8_t pct) {
    if (pct > 100) pct = 100;
    if ((int)pct == batt_shown) return;
    batt_shown = pct;
    char num[4];
    snprintf(num, sizeof(num), "%u", pct);
    int n = strlen(num);
    bool show_pct = pct < 100;
    int total = n * BATT_PITCH - 1 + (show_pct ? BPCT_W : 0);
    int vx = 16 - total / 2;
    for (int i = 0; i < 3; i++) {
        if (i < n) {
            lv_image_set_src(batt_chars[i], big_digit[num[i] - '0']);
            lv_obj_set_pos(batt_chars[i], BATT_NX, vx);
            set_hidden(batt_chars[i], false);
            vx += BATT_PITCH;
        } else {
            set_hidden(batt_chars[i], true);
        }
    }
    if (show_pct) {
        lv_obj_set_pos(batt_pct_obj, BPCT_NX, vx);
        set_hidden(batt_pct_obj, false);
    } else {
        set_hidden(batt_pct_obj, true);
    }
}

static void tick_cb(lv_timer_t *timer) {
    render_pulse();
    bool linked = zmk_split_bt_peripheral_is_connected();
    lv_image_set_src(rune_img, linked ? &rune_ok : &rune_x);
    render_batt(zmk_battery_state_of_charge());
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *bg = lv_img_create(screen);
    lv_image_set_src(bg, &bg_right);
    lv_obj_set_pos(bg, 0, 0);

    make_pulse(screen);

    for (int i = 0; i < 3; i++) batt_chars[i] = lv_img_create(screen);
    batt_pct_obj = lv_img_create(screen);
    lv_image_set_src(batt_pct_obj, &dig_pct);

    rune_img = lv_img_create(screen);
    lv_obj_set_pos(rune_img, LINK_NX, LINK_NY);

    lv_timer_create(tick_cb, TICK_MS, NULL);
    tick_cb(NULL);
    return screen;
}

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
