/*
 * Custom Corne status screens, composed for the 90°-CCW-mounted panels
 * (all art in screen_art.h is pre-rotated to native 128x32 coordinates).
 *
 * Left (central) — "Float": active layer as a boxed initial floating in a
 * dark field; bottom cluster with charging bolt, USB plug, percentage, and
 * a battery bar.
 *
 * Right (peripheral) — "Monogram": 火 emblem, link rune, battery tank,
 * percentage.
 *
 * All dynamic content is image-src swaps, hidden flags, and rectangle
 * resizes on const data — nothing draws into pixel buffers at runtime. A
 * single slow timer polls local ZMK state getters on the LVGL thread, so
 * no cross-thread event marshalling is needed.
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/battery.h>
#include <lvgl.h>

#include "screen_art.h"

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/keymap.h>
#include <zmk/usb.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#endif

#define TICK_MS 500
#define PCT_SLOTS 4

static const lv_img_dsc_t *const digit_dsc[10] = {
    &dig_0, &dig_1, &dig_2, &dig_3, &dig_4, &dig_5, &dig_6, &dig_7, &dig_8, &dig_9,
};

static lv_obj_t *pct_chars[PCT_SLOTS];
static lv_obj_t *fill_rect;

static void set_hidden(lv_obj_t *obj, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

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

/* Percentage row: tiny pre-rotated glyphs share one native x; the viewer-x
 * (= native y) of each char is computed so the string centers on center_vx. */
static void set_pct_row(uint8_t pct, int native_x, int center_vx) {
    char buf[PCT_SLOTS + 1];
    snprintf(buf, sizeof(buf), "%u%%", pct > 100 ? 100 : pct);
    int len = strlen(buf);
    int vx = center_vx - (len * 4 - 1) / 2;
    for (int i = 0; i < PCT_SLOTS; i++) {
        if (i < len) {
            lv_image_set_src(pct_chars[i], buf[i] == '%' ? &dig_pct : digit_dsc[buf[i] - '0']);
            lv_obj_set_pos(pct_chars[i], native_x, vx + i * 4);
            set_hidden(pct_chars[i], false);
        } else {
            set_hidden(pct_chars[i], true);
        }
    }
}

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

/* ── Left · Float ── */

static lv_obj_t *ltr_img;
static lv_obj_t *bolt_img;
static lv_obj_t *plug_img;

static void tick_cb(lv_timer_t *timer) {
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
    if (fw > 0) {
        lv_obj_set_size(fill_rect, 3, fw);
    }
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

    for (int i = 0; i < PCT_SLOTS; i++) {
        pct_chars[i] = lv_img_create(screen);
    }

    lv_timer_create(tick_cb, TICK_MS, NULL);
    tick_cb(NULL);

    return screen;
}

#else /* peripheral */

/* ── Right · Monogram ── */

static lv_obj_t *rune_img;

static void tick_cb(lv_timer_t *timer) {
    bool linked = zmk_split_bt_peripheral_is_connected();
    lv_image_set_src(rune_img, linked ? &rune_ok : &rune_x);

    uint8_t pct = zmk_battery_state_of_charge();
    set_pct_row(pct, RPCT_NX, 16);
    int fh = RTANK_MAX * pct / 100;
    set_hidden(fill_rect, fh == 0);
    if (fh > 0) {
        lv_obj_set_size(fill_rect, fh, 8);
    }
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *bg = lv_img_create(screen);
    lv_image_set_src(bg, &bg_right);
    lv_obj_set_pos(bg, 0, 0);

    rune_img = lv_img_create(screen);
    lv_obj_set_pos(rune_img, RUNE_X, RUNE_Y);

    fill_rect = make_fill(screen);
    lv_obj_set_pos(fill_rect, RTANK_NX, RTANK_NY);

    for (int i = 0; i < PCT_SLOTS; i++) {
        pct_chars[i] = lv_img_create(screen);
    }

    lv_timer_create(tick_cb, TICK_MS, NULL);
    tick_cb(NULL);

    return screen;
}

#endif /* IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) */
