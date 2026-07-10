/*
 * Custom Corne peripheral (right half) display:
 * pixel cat (rotated 90° CW, right side) + battery/BT widgets.
 *
 * The image declaration mirrors ZMK's in-tree nice_view art (LVGL I1
 * format: 8-byte palette followed by rows padded to whole bytes), and the
 * cat is only ever swapped between two const images — no runtime pixel
 * mutation, so nothing here depends on LVGL decoder internals.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/peripheral_status.h>
#include <lvgl.h>

/* ── Cat, 20x24 (28x20 art rotated 90° CW), two frames (open / blink) ── */

#define CAT_W 20
#define CAT_H 24

static const uint8_t cat_open_map[] = {
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00,
    0x80, 0x00, 0x63, 0xfc, 0x00, 0x94, 0x03, 0x00, 0x88, 0x00, 0xf0, 0x88,
    0x18, 0x30, 0x88, 0x98, 0x60, 0x89, 0x00, 0xc0, 0x89, 0x00, 0x80, 0x89,
    0xc0, 0x80, 0x89, 0xc0, 0x80, 0x88, 0x00, 0x80, 0x88, 0x00, 0xc0, 0x88,
    0x98, 0x60, 0x94, 0x18, 0x30, 0xe3, 0x00, 0xf0, 0x80, 0x03, 0x00, 0x40,
    0xfc, 0x00, 0x20, 0x80, 0x00, 0x10, 0x40, 0x00, 0x10, 0x00, 0x00, 0xd0,
    0x00, 0x00, 0x90, 0x00, 0x00, 0x60, 0x00, 0x00,
};

static const uint8_t cat_blink_map[] = {
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00,
    0x80, 0x00, 0x63, 0xfc, 0x00, 0x94, 0x03, 0x00, 0x88, 0x00, 0xf0, 0x88,
    0x10, 0x30, 0x88, 0x90, 0x60, 0x89, 0x00, 0xc0, 0x89, 0x00, 0x80, 0x89,
    0xc0, 0x80, 0x89, 0xc0, 0x80, 0x88, 0x00, 0x80, 0x88, 0x00, 0xc0, 0x88,
    0x90, 0x60, 0x94, 0x10, 0x30, 0xe3, 0x00, 0xf0, 0x80, 0x03, 0x00, 0x40,
    0xfc, 0x00, 0x20, 0x80, 0x00, 0x10, 0x40, 0x00, 0x10, 0x00, 0x00, 0xd0,
    0x00, 0x00, 0x90, 0x00, 0x00, 0x60, 0x00, 0x00,
};

static const lv_img_dsc_t cat_open = {
    .header.cf = LV_COLOR_FORMAT_I1,
    .header.w = CAT_W,
    .header.h = CAT_H,
    .data_size = sizeof(cat_open_map),
    .data = cat_open_map,
};

static const lv_img_dsc_t cat_blink = {
    .header.cf = LV_COLOR_FORMAT_I1,
    .header.w = CAT_W,
    .header.h = CAT_H,
    .data_size = sizeof(cat_blink_map),
    .data = cat_blink_map,
};

/* ── Periodic update: blink the cat ── */

#define TICK_MS 150
#define BLINK_PERIOD_TICKS 24 /* blink for one tick every ~3.6 s */

static lv_obj_t *cat_img;

static void tick_cb(lv_timer_t *timer) {
    static uint32_t tick;

    tick++;
    lv_image_set_src(cat_img, (tick % BLINK_PERIOD_TICKS) == 0 ? &cat_blink : &cat_open);
}

/* ── Screen layout (128x32) ──
 *
 * +-----------+----------------+---------+
 * | BT status |                |         |
 * +-----------+                |  (cat)  |
 * | Battery   |                |         |
 * +-----------+----------------+---------+
 */

static struct zmk_widget_battery_status battery_widget;
static struct zmk_widget_peripheral_status peripheral_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    zmk_widget_peripheral_status_init(&peripheral_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    zmk_widget_battery_status_init(&battery_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    cat_img = lv_img_create(screen);
    lv_image_set_src(cat_img, &cat_open);
    lv_obj_align(cat_img, LV_ALIGN_RIGHT_MID, -2, 0);

    lv_timer_create(tick_cb, TICK_MS, NULL);

    return screen;
}
