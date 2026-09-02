#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "main.h"
#include "lock_logos.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

static lv_obj_t *create_info_row(lv_obj_t *parent, const char *key, const char *value) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(row, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_cross_place(row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_key = lv_label_create(row);
    lv_obj_set_width(lbl_key, 155);
    lv_obj_set_height(lbl_key, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl_key, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_key, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl_key, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl_key, LV_LABEL_LONG_CLIP);
    lv_label_set_text(lbl_key, key);

    lv_obj_t *lbl_val = lv_label_create(row);
    lv_obj_set_flex_grow(lbl_val, 1);
    lv_obj_set_height(lbl_val, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(lbl_val, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl_val, value);
    return lbl_val;
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 800, 45);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff393939), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // Logotipo Welltepp (antes era un label de texto "WELLTEP";
                    // ahora usa la misma imagen wordmark de la pantalla de bloqueo,
                    // reducida al 80% para que entre mejor en la barra superior).
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.obj1 = obj;
                    lv_img_set_src(obj, &lock_logo_wordmark);
                    lv_image_set_scale(obj, 205); // 205/256 ≈ 80%
                    lv_obj_set_pos(obj, -8, 13);
                }
                {
                    // Pill "Modelo de robot" — arranca oculto (a diferencia de
                    // Online/Bluetooth, que siempre muestran su cuadro con el LED
                    // apagado): recien aparece cuando llega el primer
                    // 0x25 HMI_REG_ROBOT_MODEL (ver hmi_handle_reg en main.c).
                    // Se ubica ANTES de Online, en el hueco libre entre el logo y
                    // el pill de Online.
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.robot_model_pill = obj;
                    // x=306: el pill de Online se ensancha en runtime
                    // (hmi_conn_indicator_create() en main.c lo mueve a
                    // x=427 para el LED de conexion HMI<->consola). Con
                    // ancho 106, el borde derecho queda en 412, dejando
                    // 15px de separacion con Online (igual que Online-BT).
                    lv_obj_set_pos(obj, 306, 8);
                    lv_obj_set_size(obj, 106, 30);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // led_robot_model
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.led_robot_model = obj;
                            lv_obj_set_pos(obj, 9, 5);
                            lv_obj_set_size(obj, 12, 12);
                            lv_led_set_color(obj, lv_color_hex(0xfff5c518));
                            lv_led_set_brightness(obj, 255);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.robot_model_label = obj;
                            lv_obj_set_pos(obj, 29, 5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "RD--");
                        }
                    }
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 451, 8);
                    lv_obj_set_size(obj, 106, 30);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_FOCUSED);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // led_online
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.led_online = obj;
                            lv_obj_set_pos(obj, 9, 5);
                            lv_obj_set_size(obj, 12, 12);
                            lv_led_set_color(obj, lv_color_hex(0xff00971c));
                            lv_led_set_brightness(obj, 0);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj3 = obj;
                            lv_obj_set_pos(obj, 29, 5);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Online");
                        }
                    }
                }
                {
                    // console_voltage_percent_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.console_voltage_percent_label = obj;
                    lv_obj_set_pos(obj, 705, 15);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "0%");
                }
                {
                    // console_voltage_percent_bar
                    lv_obj_t *obj = lv_bar_create(parent_obj);
                    objects.console_voltage_percent_bar = obj;
                    lv_obj_set_pos(obj, 753, 20);
                    lv_obj_set_size(obj, 36, LV_PCT(15));
                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, 572, 8);
                    lv_obj_set_size(obj, 120, 30);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_FOCUSED);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // led_bluetooth
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.led_bluetooth = obj;
                            lv_obj_set_pos(obj, 9, 5);
                            lv_obj_set_size(obj, 12, 12);
                            lv_led_set_color(obj, lv_color_hex(0xff0034ff));
                            lv_led_set_brightness(obj, 0);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.obj5 = obj;
                            lv_obj_set_pos(obj, 35, 4);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Bluetooth");
                        }
                    }
                }
            }
        }
        {
            // tabview
            lv_obj_t *obj = lv_tabview_create(parent_obj);
            objects.tabview = obj;
            lv_obj_set_pos(obj, 0, 45);
            lv_obj_set_size(obj, 800, 435);
            lv_tabview_set_tab_bar_position(obj, LV_DIR_TOP);
            lv_tabview_set_tab_bar_size(obj, 50);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffbc0f2d), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // general
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "GENERAL CONTROLS");
                    objects.general = obj;
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj6 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, 372, 210);
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj7 = obj;
                                    lv_obj_set_pos(obj, 10, 40);
                                    lv_obj_set_size(obj, LV_PCT(90), LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " SYSTEM BATERRY");
                                }
                                {
                                    // robot_voltage_percent_bar
                                    lv_obj_t *obj = lv_bar_create(parent_obj);
                                    objects.robot_voltage_percent_bar = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, 86, 123);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2d2d2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff27ae60), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *alert = lv_label_create(obj);
                                        objects.robot_battery_alert = alert;
                                        lv_obj_set_width(alert, LV_PCT(100));
                                        lv_obj_set_height(alert, LV_SIZE_CONTENT);
                                        lv_obj_set_style_text_color(alert, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(alert, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(alert, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_long_mode(alert, LV_LABEL_LONG_WRAP);
                                        lv_label_set_text(alert, "");
                                        lv_obj_align(alert, LV_ALIGN_CENTER, 0, 0);
                                        lv_obj_add_flag(alert, LV_OBJ_FLAG_HIDDEN);
                                    }
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, 139, -138);
                                    lv_obj_set_size(obj, 174, 132);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.robot_voltage_caption = obj;
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Robot Voltage");
                                        }
                                        {
                                            // robot_voltage
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.robot_voltage = obj;
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "0.0 V");
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.console_voltage_caption = obj;
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Console Voltage");
                                        }
                                        {
                                            // console_voltage
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.console_voltage = obj;
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "0.0 V");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj17 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, 367, 210);
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj18 = obj;
                                    lv_obj_set_pos(obj, 18, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " ENCODER");
                                }
                                {
                                    // encoder_value — el selector Pies/Metros se movio a
                                    // Settings > Encoder (main.c, encoder_display_toggle_create),
                                    // asi que este numero ahora tiene toda la tarjeta para el
                                    // solo: caja e fuente mas grandes.
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.encoder_value = obj;
                                    lv_obj_set_pos(obj, 24, 45);
                                    lv_obj_set_size(obj, 320, 70);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 63, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "0.000 m");
                                }
                                {
                                    // btn_encoder_reset
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_encoder_reset = obj;
                                    lv_obj_set_pos(obj, 133, 135);
                                    lv_obj_set_size(obj, 100, 50);
                                    lv_obj_add_event_cb(obj, action_encoder_button_reset_clicked, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "RESET");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj14 = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, 760, 120);
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj15 = obj;
                                    lv_obj_set_pos(obj, 18, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " ROBOT LED BRIGHTNESS");
                                }
                                {
                                    lv_obj_t *obj = lv_slider_create(parent_obj);
                                    objects.obj16 = obj;
                                    lv_obj_set_pos(obj, 0, 10);
                                    lv_obj_set_size(obj, LV_PCT(90), 40);
                                    lv_obj_add_event_cb(obj, action_robot_brightness_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
                                    lv_obj_add_event_cb(obj, action_robot_brightness_released, LV_EVENT_RELEASED, (void *)0);
                                    lv_obj_add_state(obj, LV_STATE_PRESSED);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    /* Efecto esfera 3D: destello concentrado arriba, sombra concentrada abajo */
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc8c8c8), LV_PART_KNOB | LV_STATE_DEFAULT);      /* color destello (punto de luz) */
                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff0f0f0f), LV_PART_KNOB | LV_STATE_DEFAULT); /* color sombra */
                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_KNOB | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_main_stop(obj, 50,  LV_PART_KNOB | LV_STATE_DEFAULT); /* donde termina el destello (0-255): menor=mas concentrado arriba */
                                    lv_obj_set_style_bg_grad_stop(obj, 210, LV_PART_KNOB | LV_STATE_DEFAULT); /* donde empieza la sombra (0-255): mayor=mas concentrado abajo */
                                    lv_obj_set_style_bg_opa(obj, 230, LV_PART_KNOB | LV_STATE_DEFAULT); /* 0=transparente 255=solido */
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_KNOB | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 180, LV_PART_KNOB | LV_STATE_DEFAULT); /* difuminado borde */
                                    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_width(obj, 18, LV_PART_KNOB | LV_STATE_DEFAULT); /* halo difuso alrededor */
                                    lv_obj_set_style_shadow_opa(obj, 130, LV_PART_KNOB | LV_STATE_DEFAULT); /* intensidad halo: 0=sin halo 255=intenso */
                                    /* Tamaño del circulo knob: pad_all expande el knob sobre el alto del slider (40px,
                                     * igualado a Settings > Brightness > Screen para que el grosor de ambos matchee).
                                     * Formula: diametro = 40 + (pad × 2)
                                     *   pad =  0 → 40px  (mismo que la barra)
                                     *   pad =  5 → 50px
                                     *   pad = 10 → 60px  (grande, igual que el slider de Settings)
                                     *   pad = -5 → 30px  (mas pequeño que la barra) */
                                    lv_obj_set_style_pad_all(obj, 10, LV_PART_KNOB | LV_STATE_DEFAULT);
                                    lv_obj_set_style_outline_width(obj, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                    lv_obj_set_style_outline_opa(obj, 127, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                }
                                {
                                    // brightness_label
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.brightness_label = obj;
                                    lv_obj_set_pos(obj, -10, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "0%");
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj10 = obj;
                            lv_obj_set_pos(obj, 13, 215);
                            lv_obj_set_size(obj, 760, 119);
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj11 = obj;
                                    lv_obj_set_pos(obj, 16, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_post(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " TILT");
                                }
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj12 = obj;
                                    lv_obj_set_pos(obj, 60, 50);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Angle X");
                                }
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj13 = obj;
                                    lv_obj_set_pos(obj, 500, 50);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "Angle Y");
                                }
                                {
                                    // angle_x
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.angle_x = obj;
                                    lv_obj_set_pos(obj, 180, 38);
                                    lv_obj_set_size(obj, 110, 42);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 63, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "0 °");
                                }
                                {
                                    // angle_y
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.angle_y = obj;
                                    lv_obj_set_pos(obj, 620, 38);
                                    lv_obj_set_size(obj, 110, 42);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 63, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, "0 °");
                                }
                            }
                        }
                    }
                }
                {
                    // paneo
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "MODES");
                    objects.paneo = obj;
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_MOMENTUM);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_AROUND, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj19 = obj;
                            objects.modes_neck_panel = obj;
                            lv_obj_set_pos(obj, 0, 0);
                            lv_obj_set_size(obj, LV_PCT(48), LV_PCT(50));
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj20 = obj;
                                    lv_obj_set_pos(obj, 18, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " NECK START ANGLE");
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, 18, 45);
                                    lv_obj_set_size(obj, 320, 105);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // angle_neck_btn_decrese
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.angle_neck_btn_decrese = obj;
                                            lv_obj_set_pos(obj, -19, 2);
                                            lv_obj_set_size(obj, 70, 55);
                                            lv_obj_add_event_cb(obj, action_angle_neck_btn_decreased, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_event_cb(obj, action_angle_neck_btn_decreased_pressing, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "-");
                                                }
                                            }
                                        }
                                        {
                                            // angle_neck_label
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.angle_neck_label = obj;
                                            lv_obj_set_pos(obj, 0, 0);
                                            lv_obj_set_size(obj, 147, 45);
                                            lv_obj_add_event_cb(obj, action_angle_neck_label_pressed, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 63, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "090");
                                        }
                                        {
                                            // angle_neck_btn_increased
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.angle_neck_btn_increased = obj;
                                            lv_obj_set_pos(obj, 414, 57);
                                            lv_obj_set_size(obj, 70, 55);
                                            lv_obj_add_event_cb(obj, action_angle_neck_btn_increased, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_event_cb(obj, action_angle_neck_btn_increased_pressing, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "+");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj21 = obj;
                            objects.modes_head_panel = obj;
                            lv_obj_set_pos(obj, -195, 0);
                            lv_obj_set_size(obj, LV_PCT(48), LV_PCT(50));
                            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    objects.obj22 = obj;
                                    lv_obj_set_pos(obj, 18, 10);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(obj, " HEAD START ANGLE");
                                }
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_pos(obj, 18, 45);
                                    lv_obj_set_size(obj, 320, 105);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            // angle_head_btn_decrese
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.angle_head_btn_decrese = obj;
                                            lv_obj_set_pos(obj, 12, 20);
                                            lv_obj_set_size(obj, 70, 55);
                                            lv_obj_add_event_cb(obj, action_angle_head_btn_decreased, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_event_cb(obj, action_angle_head_btn_decreased_pressing, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "-");
                                                }
                                            }
                                        }
                                        {
                                            // angle_head_label
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.angle_head_label = obj;
                                            lv_obj_set_pos(obj, 614, -1239);
                                            lv_obj_set_size(obj, 147, 45);
                                            lv_obj_add_event_cb(obj, action_angle_head_label_pressed, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_opa(obj, 63, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff008000), LV_PART_MAIN | LV_STATE_PRESSED);
                                            lv_label_set_text(obj, "090");
                                        }
                                        {
                                            // angle_head_btn_increased
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            objects.angle_head_btn_increased = obj;
                                            lv_obj_set_pos(obj, 267, 26);
                                            lv_obj_set_size(obj, 70, 55);
                                            lv_obj_add_event_cb(obj, action_angle_head_btn_increased, LV_EVENT_PRESSED, (void *)0);
                                            lv_obj_add_event_cb(obj, action_angle_head_btn_increased_pressing, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "+");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        {
                            // modes_grid_panel — grilla 2 filas x 3 columnas.
                            // Se construye aca mismo, sincronicamente, junto
                            // con el resto de create_screens() (ver
                            // boot_deferred_construction en la memoria del
                            // proyecto: nada de esto se escalona en timers).
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.modes_grid_panel = obj;
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_row(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // btn_start_demo — fila 1, columna 1
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_start_demo = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, action_btn_start_demo, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj24 = obj;
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "START DEMO");
                                        }
                                    }
                                }
                                {
                                    // btn_stop_demo — fila 1, columna 2
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_stop_demo = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, action_btn_stop_demo, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj25 = obj;
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "STOP DEMO");
                                        }
                                    }
                                }
                                {
                                    // btn_control_por_puntos — fila 1, columna 3
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_control_por_puntos = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, modes_btn_control_por_puntos_cb, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "CONTROL POR\nPUNTOS");
                                        }
                                    }
                                }
                                {
                                    // btn_config_auto_rotation — fila 2, columna 1
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_config_auto_rotation = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, modes_btn_config_auto_rotation_cb, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "CONFIG AUTO\nROTATION");
                                        }
                                    }
                                }
                                {
                                    // btn_giro_automatico — fila 2, columna 2 (alterna Iniciar/Pausar en runtime)
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_giro_automatico = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, modes_btn_giro_automatico_cb, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            objects.obj23 = obj;
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "INICIAR GIRO\nAUTOMATICO");
                                        }
                                    }
                                }
                                {
                                    // btn_stop_giro_automatico — fila 2, columna 3 (Cancelar)
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.btn_stop_giro_automatico = obj;
                                    lv_obj_set_size(obj, LV_PCT(31), 90);
                                    lv_obj_add_event_cb(obj, modes_btn_stop_giro_automatico_cb, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "STOP GIRO\nAUTOMATICO");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    // configuracion
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "SETTINGS");
                    objects.configuracion = obj;
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_MOMENTUM);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Legacy hidden labels (maintained for UART update compatibility)
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bluetooth_password_label = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_label_set_text(obj, "000000");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.serial_number = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_label_set_text(obj, "---");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.robot_serial_number = obj;
                            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                            lv_label_set_text(obj, "---");
                        }
                        // Left navigation column
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj26 = obj;
                            lv_obj_set_size(obj, 145, LV_PCT(100));
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1e1e1e), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_left(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                // Brightness button (active by default)
                                {
                                    lv_obj_t *obj = lv_btn_create(parent_obj);
                                    objects.settings_btn_brightness = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_btn_brightness, LV_EVENT_CLICKED, (void *)0);
                                    {
                                        lv_obj_t *lbl = lv_label_create(obj);
                                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_text(lbl, "Brightness");
                                    }
                                }
                                // Theme button (inactive)
                                {
                                    lv_obj_t *obj = lv_btn_create(parent_obj);
                                    objects.settings_btn_theme = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_btn_theme, LV_EVENT_CLICKED, (void *)0);
                                    {
                                        lv_obj_t *lbl = lv_label_create(obj);
                                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_text(lbl, "Theme");
                                    }
                                }
                                // Battery button (inactive)
                                {
                                    lv_obj_t *obj = lv_btn_create(parent_obj);
                                    objects.settings_btn_battery = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_btn_battery, LV_EVENT_CLICKED, (void *)0);
                                    {
                                        lv_obj_t *lbl = lv_label_create(obj);
                                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_text(lbl, "Battery");
                                    }
                                }
                                // Language button (inactive)
                                {
                                    lv_obj_t *obj = lv_btn_create(parent_obj);
                                    objects.settings_btn_language = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_btn_language, LV_EVENT_CLICKED, (void *)0);
                                    {
                                        lv_obj_t *lbl = lv_label_create(obj);
                                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_text(lbl, "Language");
                                    }
                                }
                                // User button (inactive)
                                {
                                    lv_obj_t *obj = lv_btn_create(parent_obj);
                                    objects.settings_btn_user = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_btn_user, LV_EVENT_CLICKED, (void *)0);
                                    {
                                        lv_obj_t *lbl = lv_label_create(obj);
                                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                        lv_label_set_text(lbl, "User");
                                    }
                                }
                            }
                        }
                        // Right content area
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            objects.obj27 = obj;
                            lv_obj_set_flex_grow(obj, 1);
                            lv_obj_set_height(obj, LV_PCT(100));
                            lv_obj_set_style_pad_left(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                // --- BRIGHTNESS PANEL (visible by default) ---
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.settings_content_brightness = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        // Title
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, " BRIGHTNESS");
                                        }
                                        // Slider row container
                                        {
                                            lv_obj_t *obj = lv_obj_create(parent_obj);
                                            objects.obj28 = obj;
                                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_ver(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                            lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                // "Brightness" key label
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "Screen :");
                                                }
                                                // console_brightness_slider
                                                {
                                                    lv_obj_t *obj = lv_slider_create(parent_obj);
                                                    objects.console_brightness_slider = obj;
                                                    lv_obj_set_flex_grow(obj, 1);
                                                    lv_obj_set_height(obj, 40);
                                                    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
                                                    lv_slider_set_value(obj, 50, LV_ANIM_OFF);
                                                    lv_obj_add_event_cb(obj, action_console_brightness_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
                                                    lv_obj_add_event_cb(obj, action_console_brightness_released, LV_EVENT_RELEASED, (void *)0);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff9f8d11), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                                                    /* Efecto esfera 3D: destello concentrado arriba, sombra concentrada abajo */
                                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffc8c8c8), LV_PART_KNOB | LV_STATE_DEFAULT);      /* color destello (punto de luz) */
                                                    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff0f0f0f), LV_PART_KNOB | LV_STATE_DEFAULT); /* color sombra */
                                                    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_KNOB | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_bg_main_stop(obj, 50,  LV_PART_KNOB | LV_STATE_DEFAULT); /* donde termina el destello (0-255): menor=mas concentrado arriba */
                                                    lv_obj_set_style_bg_grad_stop(obj, 210, LV_PART_KNOB | LV_STATE_DEFAULT); /* donde empieza la sombra (0-255): mayor=mas concentrado abajo */
                                                    lv_obj_set_style_bg_opa(obj, 230, LV_PART_KNOB | LV_STATE_DEFAULT); /* 0=transparente 255=solido */
                                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_KNOB | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_width(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_border_opa(obj, 180, LV_PART_KNOB | LV_STATE_DEFAULT); /* difuminado borde */
                                                    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, LV_PART_KNOB | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_shadow_width(obj, 18, LV_PART_KNOB | LV_STATE_DEFAULT); /* halo difuso alrededor */
                                                    lv_obj_set_style_shadow_opa(obj, 130, LV_PART_KNOB | LV_STATE_DEFAULT); /* intensidad halo: 0=sin halo 255=intenso */
                                                    /* Tamaño del circulo knob: pad_all expande el knob sobre el alto del slider (28px).
                                                     * Formula: diametro = 28 + (pad × 2)
                                                     *   pad =  0 → 28px  (mismo que la barra)
                                                     *   pad =  5 → 38px
                                                     *   pad = 10 → 48px  (grande)
                                                     *   pad = -5 → 18px  (mas pequeño que la barra) */
                                                    lv_obj_set_style_pad_all(obj, 10, LV_PART_KNOB | LV_STATE_DEFAULT);
                                                }
                                                // console_brightness_label
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    objects.console_brightness_label = obj;
                                                    lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text(obj, "50 %");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                {
                    // sysinfo
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "SYSTEM INFO");
                    objects.sysinfo = obj;
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_MOMENTUM);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // Left navigation column
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, 145, LV_PCT(100));
                            lv_obj_set_style_pad_left(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_row(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // sysinfo_btn_device (active by default)
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.sysinfo_btn_device = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_add_event_cb(obj, action_sysinfo_btn_device, LV_EVENT_CLICKED, NULL);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Device");
                                        }
                                    }
                                }
                                {
                                    // sysinfo_btn_version
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.sysinfo_btn_version = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_add_event_cb(obj, action_sysinfo_btn_version, LV_EVENT_CLICKED, NULL);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Version");
                                        }
                                    }
                                }
                                {
                                    // sysinfo_btn_guide
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.sysinfo_btn_guide = obj;
                                    lv_obj_set_size(obj, LV_PCT(100), 60);
                                    lv_obj_add_event_cb(obj, action_sysinfo_btn_guide, LV_EVENT_CLICKED, NULL);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, "Guide");
                                        }
                                    }
                                }
                            }
                        }
                        {
                            // Right content area
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_flex_grow(obj, 1);
                            lv_obj_set_height(obj, LV_PCT(100));
                            lv_obj_set_style_pad_left(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_top(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_right(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_bottom(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // sysinfo_content_device (visible by default)
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    objects.sysinfo_content_device = obj;
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                                            lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_label_set_text(obj, " DEVICE INFORMATION");
                                        }
                                        objects.sysinfo_device_name_value = create_info_row(parent_obj, "Device Name :", "WELLTEP Console");
                                        create_info_row(parent_obj, "Model :", "RD90C");
                                        objects.sysinfo_console_serial_value = create_info_row(parent_obj, "Serial :", "---");
                                        objects.sysinfo_robot_serial_value = create_info_row(parent_obj, "Robot S/N :", "---");
                                        create_info_row(parent_obj, "Manufacturer :", "Welltep Robotics");
                                        create_info_row(parent_obj, "Display :", "4\" MIPI DSI 800x480");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    tick_screen_main();
}

// Todos los paneles de contenido que empiezan OCULTOS (LV_OBJ_FLAG_HIDDEN) en
// Settings (Encoder/Tecnologia/Theme/Battery/Language/User) y System Info
// (Update/Version/Guide) se arman aca, aparte de create_screen_main(), asi el
// arbol grande de create_screen_main() queda mas chico y ordenado. Ninguno de
// estos paneles se ve hasta que el usuario toca su boton de nav. Los
// contenedores padre se resuelven con lv_obj_get_parent() sobre botones/
// paneles que SI siguen creandose en create_screen_main() (settings_btn_user,
// settings_content_brightness, sysinfo_btn_guide, sysinfo_content_device),
// porque este codigo ya no esta anidado dentro de ese arbol. Todas estas
// funciones se llaman seguidas, de un tiron, desde ui_init() (ui.c) — ver la
// memoria del proyecto (boot_deferred_construction): escalonarlas con locks o
// timers separados (para que la interfaz base aparezca antes) se probo varias
// veces y siempre termino en pantalla negra trabada, asi que no se debe
// repetir sin antes conseguir un log real del cuelgue.
void create_panel_settings_encoder() {
    // ---- Settings > Encoder ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        // Encoder button (inactive)
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn_encoder = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_settings_btn_encoder, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Encoder");
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // Encoder panel (hidden by default)
        // NOTA: bloques {} anidados a proposito (en vez de variables locales
        // planas) para que el compilador libere cada slot de stack al cerrar
        // el bloque. objects.enc_spinbox_perim/enc_spinbox_ppr se usan
        // directamente en vez de variables locales para que los botones de
        // flechas los referencien sin mantener el spinbox "vivo" fuera de su
        // propio bloque.
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_encoder = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " ENCODER");
                }
                // Subtitulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Encoder roller perimeter. Formula: Dist.(m) = Pulses x Perimeter / 1000  |  Default: 85.20 mm");
                }
                // Fila Perimetro
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_ver(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Key
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_size(obj, 130, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Perimeter:");
                        }
                        // Wrap: spinbox + flechas
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                // Spinbox
                                {
                                    lv_obj_t *obj = lv_spinbox_create(parent_obj);
                                    objects.enc_spinbox_perim = obj;
                                    lv_spinbox_set_digit_format(obj, 5, 3);
                                    lv_spinbox_set_range(obj, 100, 99999);
                                    lv_spinbox_set_step(obj, 10);
                                    lv_spinbox_set_value(obj, 8520);
                                    lv_obj_set_size(obj, 104, 44);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 102, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                                // Columna de flechas
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_size(obj, 36, 44);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_row(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        // Flecha arriba
                                        {
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            lv_obj_set_size(obj, 36, 21);
                                            lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, enc_sb_increment_cb, LV_EVENT_CLICKED, objects.enc_spinbox_perim);
                                            {
                                                lv_obj_t *lbl = lv_label_create(obj);
                                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                lv_label_set_text(lbl, LV_SYMBOL_UP);
                                            }
                                        }
                                        // Flecha abajo
                                        {
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            lv_obj_set_size(obj, 36, 21);
                                            lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, enc_sb_decrement_cb, LV_EVENT_CLICKED, objects.enc_spinbox_perim);
                                            {
                                                lv_obj_t *lbl = lv_label_create(obj);
                                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                lv_label_set_text(lbl, LV_SYMBOL_DOWN);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // Unidad
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "mm");
                        }
                    }
                }
                // Presets rapidos
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        static const int32_t enc_preset_values[] = { 8520, 10000, 12500, 15000, 20000 };
                        static const char *enc_preset_labels[]   = { "85.2", "100.0", "125.0", "150.0", "200.0" };
                        for (size_t enc_pi = 0; enc_pi < 5; enc_pi++) {
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            lv_obj_set_size(obj, 90, 44);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, enc_preset_cb, LV_EVENT_CLICKED, (void *)(intptr_t)enc_preset_values[enc_pi]);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, enc_preset_labels[enc_pi]);
                            }
                        }
                    }
                }
                // Fila Pulsos/vuelta
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_ver(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Key
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_size(obj, 130, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Pulses/rev:");
                        }
                        // Wrap: spinbox + flechas
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                // Spinbox
                                {
                                    lv_obj_t *obj = lv_spinbox_create(parent_obj);
                                    objects.enc_spinbox_ppr = obj;
                                    lv_spinbox_set_digit_format(obj, 4, 0);
                                    lv_spinbox_set_range(obj, 1, 9999);
                                    lv_spinbox_set_step(obj, 1);
                                    lv_spinbox_set_value(obj, 600);
                                    lv_obj_set_size(obj, 88, 44);
                                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_opa(obj, 102, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                }
                                // Columna de flechas
                                {
                                    lv_obj_t *obj = lv_obj_create(parent_obj);
                                    lv_obj_set_size(obj, 36, 44);
                                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_pad_row(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        // Flecha arriba
                                        {
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            lv_obj_set_size(obj, 36, 21);
                                            lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, enc_sb_increment_cb, LV_EVENT_CLICKED, objects.enc_spinbox_ppr);
                                            {
                                                lv_obj_t *lbl = lv_label_create(obj);
                                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                lv_label_set_text(lbl, LV_SYMBOL_UP);
                                            }
                                        }
                                        // Flecha abajo
                                        {
                                            lv_obj_t *obj = lv_button_create(parent_obj);
                                            lv_obj_set_size(obj, 36, 21);
                                            lv_obj_set_style_radius(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                            lv_obj_add_event_cb(obj, enc_sb_decrement_cb, LV_EVENT_CLICKED, objects.enc_spinbox_ppr);
                                            {
                                                lv_obj_t *lbl = lv_label_create(obj);
                                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                lv_label_set_text(lbl, LV_SYMBOL_DOWN);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // Unidad
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "pulses/rev");
                        }
                    }
                }
                // Footer con el resultado derivado (recalculado en runtime desde main.c)
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.enc_footer_label = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
}

void create_panel_sysinfo_update() {
    // ---- System Info > Update ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.sysinfo_btn_guide);
        // Update button (inactive)
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.sysinfo_btn_update = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_add_event_cb(obj, action_sysinfo_btn_update, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Update");
                }
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.sysinfo_content_device);
        // Update panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.sysinfo_content_update = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " UPDATE");
                }
                // Fila LED + boton "Activar WiFi" — interruptor principal.
                // El LED parpadea mientras esta prendido pero no conectado a
                // ninguna red, y queda solido apenas consigue IP (logica en
                // update_led_set_mode(), main.c).
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // LED
                        {
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.update_led = obj;
                            lv_obj_set_size(obj, 14, 14);
                            lv_led_set_color(obj, lv_color_hex(0xffffffff));
                            lv_led_set_brightness(obj, 0);
                        }
                        // Boton toggle WiFi
                        {
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.update_toggle_btn = obj;
                            lv_obj_set_size(obj, 220, 50);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, update_wifi_toggle_cb, LV_EVENT_CLICKED, NULL);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Enable WiFi");
                            }
                        }
                    }
                }
                // Status label — propia fila, texto libre (Conectando.../
                // Conectado: IP/WiFi off).
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.update_status_label = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "WiFi off");
                }
                // Boton "hay actualizacion" (oculto hasta que se detecte una;
                // se muestra y se pone amarillo cuando ota_http detecta una
                // version nueva — ver ota_update_available_cb() en main.c).
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.update_available_btn = obj;
                    lv_obj_set_size(obj, 220, 50);
                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_event_cb(obj, update_confirm_cb, LV_EVENT_CLICKED, NULL);
                    {
                        lv_obj_t *lbl = lv_label_create(obj);
                        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_label_set_text(lbl, "Update");
                    }
                }
                // Info de red — oculta hasta que el WiFi este realmente
                // conectado (ver update_network_label_set_visible en main.c)
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.update_network_label = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Network: WTP TALLER");
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                }
                // Aviso de duracion aproximada de la actualizacion
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.update_duration_note = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "The update may take approximately 35 seconds");
                }
                // Red WiFi guardada (SSID/contrasena a mano) + boton Buscar
                // redes — ver hmi_open_wifi_editor() en main.c. Arranca
                // oculta: solo se muestra cuando se activa el WiFi (ver
                // update_wifi_network_row_set_visible() en main.c, llamado
                // desde hmi_wifi_set_enabled()).
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.update_wifi_network_row = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_wifi_ssid_value = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "---");
                        }
                        {
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.update_wifi_edit_btn = obj;
                            lv_obj_set_size(obj, 160, 46);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_wifi_edit, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Buscar redes");
                            }
                        }
                    }
                }
            }
        }
    }
}

void create_panel_settings_theme_battery_lang_user() {
    // ---- Settings > Theme/Battery/Language/User (paneles ocultos por defecto:
    // ninguno se ve hasta tocar su boton de nav, asi que tampoco hace falta
    // construirlos durante la ventana de pantalla negra) ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // --- THEME PANEL (hidden) ---
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_theme = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Title
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " APPEARANCE");
                }
                // Theme buttons row
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Dark Mode
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.settings_btn_theme_dark = obj;
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_theme_dark, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Dark Mode");
                            }
                        }
                        // Classic Mode (active default)
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.settings_btn_theme_classic = obj;
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_theme_classic, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Classic");
                            }
                        }
                        // Light Mode
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.settings_btn_theme_light = obj;
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_theme_light, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Light Mode");
                            }
                        }
                    }
                }
            }
        }
        // --- BATTERY PANEL (hidden) ---
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_battery = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Title
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " BATTERY DISPLAY");
                }
                // Mode selector row
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Voltage (V) — active by default
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.settings_btn_bat_voltage = obj;
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_bat_voltage, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Voltage (V)");
                            }
                        }
                        // Percentage (%) — inactive
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.settings_btn_bat_percent = obj;
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_bat_percent, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Percent (%)");
                            }
                        }
                    }
                }
            }
        }
        // Language panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_language = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Title
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " LANGUAGE");
                }
                // Language selector row
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // ES button
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.lang_btn_es = obj;
                            lv_obj_set_size(obj, 100, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_lang_es, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "ES");
                            }
                        }
                        // EN button (active by default)
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.lang_btn_en = obj;
                            lv_obj_set_size(obj, 100, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_lang_en, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "EN");
                            }
                        }
                        // PT button
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.lang_btn_pt = obj;
                            lv_obj_set_size(obj, 100, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_lang_pt, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "PT");
                            }
                        }
                    }
                }
            }
        }
        // User panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_user = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff1a1a1a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Title
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " USER");
                }
                // Device name row — mismo patron plano que la fila de Brightness
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_ver(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // "Nombre :" key label
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Name :");
                        }
                        // device name value
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.settings_user_name_value = obj;
                            lv_obj_set_flex_grow(obj, 1);
                            lv_obj_set_height(obj, LV_SIZE_CONTENT);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "WELLTEP Console");
                        }
                    }
                }
                // Action buttons row — mismo patron que la fila selectora de Battery
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Cambiar Nombre
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_user_edit_name, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Change Name");
                            }
                        }
                        // Cambiar PIN
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            lv_obj_set_size(obj, 160, 70);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_change_lock_pin, LV_EVENT_CLICKED, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "Change PIN");
                            }
                        }
                    }
                }
                // Enable-password row — mismo patron plano que la fila de Brightness
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_ver(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // "Habilitar contrasena" key label
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_flex_grow(obj, 1);
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Enable Password");
                        }
                        {
                            lv_obj_t *obj = lv_switch_create(parent_obj);
                            objects.settings_user_pin_switch = obj;
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_INDICATOR | LV_STATE_CHECKED);
                            lv_obj_add_event_cb(obj, action_settings_toggle_lock_pin, LV_EVENT_VALUE_CHANGED, (void *)0);
                        }
                    }
                }
            }
        }
    }
}

void create_panel_sysinfo_version_guide() {
    // ---- System Info > Version/Guide (paneles ocultos por defecto) ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.sysinfo_content_device);
        {
            // sysinfo_content_version (hidden)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.sysinfo_content_version = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " VERSION INFO");
                }
                create_info_row(parent_obj, "Firmware :", "---");
                {
                    // Version de firmware de la CONSOLA (llega por UART,
                    // HMI_REG_FW_VERSION) — fila propia con objeto guardado
                    // porque se actualiza en vivo (no solo una vez al boot
                    // como el resto de estas filas), ver apply_fw_version()
                    // en main.c.
                    lv_obj_t *row = create_info_row(parent_obj, "Firmware WC :", "---");
                    objects.sysinfo_console_fw_value = lv_obj_get_child(row, 1);
                }
                create_info_row(parent_obj, "Hardware :", "---");
                create_info_row(parent_obj, "LVGL :", "---");
                create_info_row(parent_obj, "ESP-IDF :", "---");
                create_info_row(parent_obj, "Build Date :", "---");
                create_info_row(parent_obj, "Boot Count :", "---");
            }
        }
        {
            // sysinfo_content_guide (hidden)
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.sysinfo_content_guide = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " QUICK GUIDE");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    // OJO: este texto es solo un placeholder de fabrica — nunca se ve.
                    // lang_apply() (ui/lang.c) pisa este label con L->guide_body
                    // como parte del boot normal (ui.c llama lang_apply() despues
                    // de create_screens()), asi que el contenido real que edita el
                    // usuario esta en lang.c (.guide_body, 3 idiomas), no aca.
                    lv_label_set_text(obj,
                        "GENERAL CONTROLS:\n"
                        "  Top bar: online/Bluetooth status and console battery.\n"
                        "  System Battery: robot battery level.\n"
                        "  Tilt: live X/Y inclination angles.\n"
                        "  Encoder: distance traveled - tap FT/METERS to switch units,\n"
                        "  RESET to zero it.\n"
                        "  Robot LED Brightness: slider for the robot's onboard LED.\n\n"
                        "MODES:\n"
                        "  Start/Stop Demo: run or stop the preset demo sequence.\n"
                        "  Control por Puntos: jog head/neck live and save a center\n"
                        "  position.\n"
                        "  Config Auto Rotation: pick Recorrido 1/2, save head/neck\n"
                        "  points, set speed (hold +/- to repeat fast), Test to preview.\n"
                        "  Iniciar Giro Automatico: play the saved recorrido; Stop Giro\n"
                        "  Automatico cancels it.\n\n"
                        "SETTINGS:\n"
                        "  Brightness, Theme (Dark/Classic/Light), Battery display\n"
                        "  (Volts/%), Language (ES/EN/PT).\n"
                        "  User: set the device name and a lock PIN for the screen.\n"
                        "  Encoder: calibrate roller perimeter and pulses/rev.\n"
                        "  Servo Limits: min/max degrees for Servo 1/2/3.\n"
                        "  Tecnologia: toggle the console's video tech LEDs.\n\n"
                        "SYSTEM INFO:\n"
                        "  Device: serial numbers and device name.\n"
                        "  Version: firmware/hardware/LVGL/ESP-IDF versions, build date,\n"
                        "  boot count.\n"
                        "  Logs: recent system log.\n"
                        "  Update: turn on this screen's WiFi to check for its own\n"
                        "  firmware update. The Console section shows the console\n"
                        "  board's own update status and lets you turn on its WiFi too.\n"
                        "  Guide: this screen."
                    );
                }
            }
        }
    }
}

void create_panel_settings_tecnologia() {
    // ---- Settings > Tecnologia ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        // Tecnologia button (inactive)
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn_tecnologia = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_settings_btn_tecnologia, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Technology");
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // Tecnologia panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_tecnologia = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " TECHNOLOGY");
                }
                // Subtitulo / descripcion
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Video technology");
                }
                // Fila de botones A/B: mientras se mantienen presionados envian 1
                // por el serial HMI (la placa de consola prende su LED
                // correspondiente), y al soltarlos envian 0.
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Boton A — el color de PRESSED lo aplica theme_technology_panel()
                        // en actions.c segun el tema activo (Dark/Classic/Light), no un
                        // amarillo fijo.
                        {
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.tech_btn_a = obj;
                            lv_obj_set_size(obj, 220, 50);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_press, LV_EVENT_PRESSED, (void *)0);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_release, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_release, LV_EVENT_PRESS_LOST, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "A");
                            }
                        }
                        // Boton B — idem
                        {
                            lv_obj_t *obj = lv_button_create(parent_obj);
                            objects.tech_btn_b = obj;
                            lv_obj_set_size(obj, 220, 50);
                            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_b_press, LV_EVENT_PRESSED, (void *)0);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_b_release, LV_EVENT_RELEASED, (void *)0);
                            lv_obj_add_event_cb(obj, action_settings_video_tech_b_release, LV_EVENT_PRESS_LOST, (void *)0);
                            {
                                lv_obj_t *lbl = lv_label_create(obj);
                                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                lv_label_set_text(lbl, "B");
                            }
                        }
                    }
                }
                // Descripcion boton A
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.tech_desc_a = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "A: Output signal change");
                }
                // Descripcion boton B
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.tech_desc_b = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "B: Output signal resolution or TV mode change");
                }
            }
        }
    }
}

// Settings > Camera — indicador de reversa: se prende (LED + cuadro
// "RETROCEDIENDO") cuando hmi_handle_reg() (main.c, case HMI_REG_MOTOR)
// detecta cmd == MOTOR_CMD_REVERSE. El cuadro arranca oculto.
void create_panel_settings_camera() {
    // ---- Settings > Camera ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        // Camera button (inactive)
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn_camera = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_settings_btn_camera, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Camera");
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // Camera panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_camera = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " CAMERA");
                }
                // Subtitulo / descripcion
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Reverse indicator, driven by the robot's motor direction");
                }
                // Fila LED + cuadro "RETROCEDIENDO"
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // LED estatico: siempre en el mismo lugar, con su halo
                            // normal de lv_led (crece con el brillo, sin recortarse
                            // porque el cuadro de al lado ya no cambia de tamano —
                            // ver nota abajo).
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.cam_reverse_led = obj;
                            lv_obj_set_size(obj, 22, 22);
                            lv_led_set_color(obj, lv_color_hex(0xffe74c3c));
                            lv_led_set_brightness(obj, 0);
                        }
                        {
                            // Caja fija (nunca se oculta ni cambia de tamano, para que
                            // la fila no cambie de alto y el LED de al lado no se corra
                            // de lugar). El texto es un HIJO centrado con lv_obj_center()
                            // (centra horizontal Y vertical, a diferencia de text_align
                            // que solo centra horizontal) para que no quede pegado
                            // arriba dentro de la caja.
                            lv_obj_t *box = lv_obj_create(parent_obj);
                            lv_obj_set_size(box, 220, 46);
                            lv_obj_set_style_bg_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(box, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_side(box, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(box, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
                            // Arranca en gris neutro; solo el COLOR del texto cambia a
                            // ambar cuando hmi_handle_reg() (main.c) detecta reversa.
                            lv_obj_t *obj = lv_label_create(box);
                            objects.cam_reverse_label = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "CAMARA #2");
                            lv_obj_center(obj);
                        }
                        // Interruptor de 3 posiciones para RL1 (solo reversa):
                        // ON (fuerza encendido) / OFF (desactivado, no manda
                        // nada) / AUTO (sigue la reversa, comportamiento de
                        // siempre). Estilo/logica/estado inicial se cablean
                        // en main.c (camera_rl1_mode_wire()), aca solo se
                        // crean los botones.
                        {
                            lv_obj_t *sw = lv_obj_create(parent_obj);
                            lv_obj_set_size(sw, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(sw, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(sw, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(sw, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(sw, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(sw, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(sw, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(sw, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = sw;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.cam_rl1_btn_on = obj;
                                    lv_obj_set_size(obj, 64, 40);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "ON");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.cam_rl1_btn_off = obj;
                                    lv_obj_set_size(obj, 64, 40);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "OFF");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.cam_rl1_btn_auto = obj;
                                    lv_obj_set_size(obj, 64, 40);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "AUTO");
                                }
                            }
                        }
                    }
                }
                // Fila LED + cuadro "AVANZANDO" — mismo patron que la de
                // arriba (RETROCEDIENDO), abajo de esa.
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // LED estatico, mismo patron que cam_reverse_led — verde
                            // en vez de ambar, para distinguir de un vistazo.
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.cam_forward_led = obj;
                            lv_obj_set_size(obj, 22, 22);
                            lv_led_set_color(obj, lv_color_hex(0xff27ae60));
                            lv_led_set_brightness(obj, 0);
                        }
                        {
                            // Caja fija, mismo patron que la de RETROCEDIENDO.
                            lv_obj_t *box = lv_obj_create(parent_obj);
                            lv_obj_set_size(box, 220, 46);
                            lv_obj_set_style_bg_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_bg_color(box, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(box, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_side(box, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_color(box, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_radius(box, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
                            // Arranca en gris neutro; solo el COLOR del texto cambia a
                            // verde cuando hmi_handle_reg() (main.c) detecta avance.
                            lv_obj_t *obj = lv_label_create(box);
                            objects.cam_forward_label = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "AVANZANDO");
                            lv_obj_center(obj);
                        }
                    }
                }
            }
        }
    }
}

void create_panel_settings_bluetooth() {
    // ---- Settings > Bluetooth — dispositivo BLE conectado a la consola
    // (MAC/estado via HMI_REG_BLUETOOTH_MAC_HI/LO/INDICATOR, contrasena via
    // HMI_REG_BLUETOOTH_PASSWORD). Boton Desconectar manda
    // HMI_REG_BLUETOOTH_DISCONNECT — logica en main.c/actions.c. ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        // Bluetooth button (inactive)
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn_bluetooth = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_settings_btn_bluetooth, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Bluetooth");
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // Bluetooth panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_bluetooth = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " BLUETOOTH");
                }
                // Subtitulo / descripcion
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Dispositivo BLE conectado a la consola");
                }
                // Fila LED + estado (Conectado / Sin conexion)
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 22, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_led_create(parent_obj);
                            objects.bt_panel_led = obj;
                            lv_obj_set_size(obj, 22, 22);
                            lv_led_set_color(obj, lv_color_hex(0xff0034ff));
                            lv_led_set_brightness(obj, 0);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bt_panel_status_label = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Sin conexion");
                        }
                    }
                }
                // Fila MAC del dispositivo conectado
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bt_panel_mac_caption = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "MAC:");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bt_panel_mac_label = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "---");
                        }
                    }
                }
                // Fila contrasena BLE
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_column(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bt_panel_password_caption = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, "Contrasena:");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.bt_panel_password_label = obj;
                            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_label_set_text(obj, get_var_bluetooth_password_string());
                        }
                    }
                }
                // Botones: 2 filas — fila 1 (Desconectar / Bloquear dispositivo),
                // fila 2 (Borrar bloqueados).
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Fila 1: Desconectar + Bloquear dispositivo
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.bt_panel_disconnect_btn = obj;
                                    lv_obj_set_size(obj, 190, 50);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffbc0f2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_bluetooth_disconnect, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Desconectar");
                                }
                                {
                                    // Bloquea (lista negra en la consola, NVS) y desconecta
                                    // el dispositivo BLE actual — HMI_REG_BLUETOOTH_BLOCK.
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.bt_panel_block_btn = obj;
                                    lv_obj_set_size(obj, 220, 50);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_bluetooth_block, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Bloquear dispositivo");
                                }
                            }
                        }
                        // Fila 2: Borrar bloqueados
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // Borra toda la lista negra de MACs bloqueadas — no hay
                                    // desbloqueo individual (ver nota junto al registro en main.h).
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.bt_panel_unblock_all_btn = obj;
                                    lv_obj_set_size(obj, 220, 50);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_add_event_cb(obj, action_settings_bluetooth_unblock_all, LV_EVENT_CLICKED, (void *)0);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Borrar bloqueados");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void create_panel_settings_joystick() {
    // ---- Settings > Joystick — que hace el boton de cada joystick al
    // presionarlo (HMI_REG_J1/J2_BUTTON_MODE). Los botones se crean vacios
    // aca; joystick_mode_wire() (main.c) les cablea el click y los deja
    // resaltados segun lo que la consola confirme (no lo que se toca). ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        // Joystick button (inactive)
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn_joystick = obj;
            lv_obj_set_size(obj, LV_PCT(100), 60);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(obj, action_settings_btn_joystick, LV_EVENT_CLICKED, (void *)0);
            {
                lv_obj_t *lbl = lv_label_create(obj);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(lbl, "Botones");
            }
        }
    }
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        // Joystick panel (hidden by default)
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.settings_content_joystick = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            {
                lv_obj_t *parent_obj = obj;
                // Titulo
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, " BOTONES");
                }
                // Subtitulo / descripcion
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                    lv_label_set_text(obj, "Que hace el boton de cada joystick al presionarlo");
                }
                // Bloque Joystick 1
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Joystick 1 (Motor)");
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Fila 1: Centrar servo3 / LED
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j1_mode_btn_servo3 = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Centrar servo3");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j1_mode_btn_led = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "LED");
                                }
                            }
                        }
                        // Fila 2: Centrar cabeza/cuello / Captura
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j1_mode_btn_center = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Centrar cabeza/cuello");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j1_mode_btn_capture = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Captura");
                                }
                            }
                        }
                    }
                }
                // Bloque Joystick 2
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Joystick 2 (Servo)");
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_row(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_left(obj, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        // Fila 1: Centrar servo3 / LED
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j2_mode_btn_servo3 = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Centrar servo3");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j2_mode_btn_led = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "LED");
                                }
                            }
                        }
                        // Fila 2: Centrar cabeza/cuello / Captura
                        {
                            lv_obj_t *obj = lv_obj_create(parent_obj);
                            lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_set_style_pad_column(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j2_mode_btn_center = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Centrar cabeza/cuello");
                                }
                                {
                                    lv_obj_t *obj = lv_button_create(parent_obj);
                                    objects.j2_mode_btn_capture = obj;
                                    lv_obj_set_size(obj, 175, 44);
                                    lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_t *lbl = lv_label_create(obj);
                                    lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text(lbl, "Captura");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// MODES > "Control por Puntos" / "Config Auto Rotation" — 3 pantallas
// completas, ocultas por defecto, hijas directas de objects.paneo (el tab
// MODES). "Pantalla completa por sub-modo": al mostrar cualquiera de estas
// 3, modes_show_view() (main.c) oculta la grilla de 6 botones. Control por
// Puntos REUBICA (no copia) los paneles +/- de cabeza/cuello originales;
// el editor de "Config Auto Rotation" tiene su propia copia separada de esos
// controles (no puede compartir los mismos objetos con Control por Puntos).
// Helper: crea una barra superior fija (titulo con borde-acento a la
// izquierda + boton de volver a la derecha) — NO participa del scroll del
// panel, asi que el boton de salir siempre esta a la vista sin impotar
// cuanto contenido tenga el cuerpo de la pantalla debajo.
static lv_obj_t *auto_rotation_make_topbar(lv_obj_t *panel_parent, lv_obj_t **out_title,
                                            const char *title_txt, lv_event_cb_t back_cb, lv_obj_t **out_back_btn) {
    lv_obj_t *bar = lv_obj_create(panel_parent);
    lv_obj_set_size(bar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_layout(bar, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_flow(bar, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_main_place(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_flex_cross_place(bar, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    {
        lv_obj_t *title = lv_label_create(bar);
        *out_title = title;
        lv_obj_set_size(title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(title, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(title, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(title, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(title, title_txt);
    }
    {
        // Outline (transparente + borde), no relleno — es una accion
        // secundaria (salir), no la primaria de la pantalla.
        lv_obj_t *btn = lv_button_create(bar);
        *out_back_btn = btn;
        lv_obj_set_size(btn, 120, 44);
        lv_obj_add_event_cb(btn, back_cb, LV_EVENT_CLICKED, (void *)0);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT " Volver");
    }
    return bar;
}

void create_panel_modes_auto_rotation() {
    // ---- Control por Puntos ----
    {
        lv_obj_t *parent_obj = objects.paneo;
        lv_obj_t *obj = lv_obj_create(parent_obj);
        objects.modes_control_puntos_panel = obj;
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        {
            lv_obj_t *panel = obj;
            auto_rotation_make_topbar(panel, &objects.cp_title_label, " CONTROL POR PUNTOS",
                                       cp_btn_volver_cb, &objects.cp_btn_volver);
            // Cuerpo scrolleable — el boton Volver queda arriba, fijo, fuera de esto.
            lv_obj_t *body = lv_obj_create(panel);
            lv_obj_set_width(body, LV_PCT(100));
            lv_obj_set_flex_grow(body, 1);
            lv_obj_set_style_bg_opa(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(body, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(body, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(body, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(body, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scroll_dir(body, LV_DIR_VER);
            {
                lv_obj_t *parent_obj = body;
                // Reubica (reparent, no copia) los paneles de angulo cuello/
                // cabeza ORIGINALES (creados en create_screens(), hoy
                // objects.modes_neck_panel / objects.modes_head_panel) —
                // mismos botones +/- y label de siempre, callbacks originales
                // intactos (action_angle_*), solo que ahora viven aca en vez
                // de en la vista home de MODES. Fila propia para que queden
                // lado a lado, igual que se veian antes.
                lv_obj_t *angle_row = lv_obj_create(parent_obj);
                lv_obj_set_size(angle_row, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(angle_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(angle_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(angle_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(angle_row, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(angle_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_column(angle_row, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(angle_row, LV_OBJ_FLAG_SCROLLABLE);
                if (objects.modes_neck_panel) {
                    lv_obj_set_parent(objects.modes_neck_panel, angle_row);
                    lv_obj_set_size(objects.modes_neck_panel, LV_PCT(49), LV_SIZE_CONTENT);
                }
                if (objects.modes_head_panel) {
                    lv_obj_set_parent(objects.modes_head_panel, angle_row);
                    lv_obj_set_size(objects.modes_head_panel, LV_PCT(49), LV_SIZE_CONTENT);
                }
                // Guardar centrado nuevo
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.cp_btn_guardar_centrado = obj;
                    lv_obj_set_size(obj, 260, 60);
                    lv_obj_add_event_cb(obj, cp_btn_guardar_centrado_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, LV_SYMBOL_SAVE " Guardar centrado nuevo"); }
                }
            }
        }
    }
    // ---- Config Auto Rotation — selector de recorrido ----
    {
        lv_obj_t *parent_obj = objects.paneo;
        lv_obj_t *obj = lv_obj_create(parent_obj);
        objects.modes_autorot_picker_panel = obj;
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        {
            lv_obj_t *panel = obj;
            auto_rotation_make_topbar(panel, &objects.autorot_picker_title_label, " CONFIG AUTO ROTATION",
                                       autorot_picker_btn_volver_cb, &objects.autorot_picker_btn_volver);
            lv_obj_t *body = lv_obj_create(panel);
            lv_obj_set_width(body, LV_PCT(100));
            lv_obj_set_flex_grow(body, 1);
            lv_obj_set_style_bg_opa(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(body, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(body, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(body, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(body, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scroll_dir(body, LV_DIR_VER);
            {
                lv_obj_t *parent_obj = body;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.autorot_picker_subtitle_label = obj;
                    lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Elegi el recorrido a editar:");
                }
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.autorot_btn_recorrido1 = obj;
                    lv_obj_set_size(obj, LV_PCT(100), 70);
                    lv_obj_add_event_cb(obj, autorot_btn_recorrido1_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, "Recorrido 1"); }
                }
                {
                    lv_obj_t *obj = lv_button_create(parent_obj);
                    objects.autorot_btn_recorrido2 = obj;
                    lv_obj_set_size(obj, LV_PCT(100), 70);
                    lv_obj_add_event_cb(obj, autorot_btn_recorrido2_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, "Recorrido 2"); }
                }
            }
        }
    }
    // ---- Config Auto Rotation — editor de puntos ----
    // Layout (ver mockup del usuario): barra superior fija (titulo+ATRAS);
    // debajo, 2 columnas: izquierda = lista de puntos + telemetria en vivo
    // (neck/head), derecha = velocidad, head (vertical +/-) + save point,
    // neck (horizontal +/-), y test — abajo del todo.
    {
        lv_obj_t *parent_obj = objects.paneo;
        lv_obj_t *obj = lv_obj_create(parent_obj);
        objects.modes_autorot_editor_panel = obj;
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Menos margen arriba/abajo que a los costados a proposito: sube
        // "RECORRIDO 1"/Volver mas cerca de la barra de pestañas, y le da a
        // "body" (y por lo tanto a right_col, que usa SPACE_BETWEEN) mas
        // alto real para repartir — empuja Test/Guardar todavia mas cerca
        // del borde de abajo.
        lv_obj_set_style_pad_top(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        {
            lv_obj_t *panel = obj;
            auto_rotation_make_topbar(panel, &objects.ar_title_label, " RECORRIDO 1",
                                       ar_btn_volver_cb, &objects.ar_btn_volver);

            lv_obj_t *body = lv_obj_create(panel);
            lv_obj_set_width(body, LV_PCT(100));
            lv_obj_set_flex_grow(body, 1);
            lv_obj_set_style_bg_opa(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(body, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(body, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(body, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_column(body, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_scroll_dir(body, LV_DIR_VER);

            // ---- Columna izquierda: lista de puntos ----
            // LV_SIZE_CONTENT (no PCT(100)): la columna mide justo lo que
            // necesita en vez de estirarse al 100% de "body". El alto de la
            // lista misma se ajusta dinamicamente a su contenido real en
            // auto_rotation_editor_refresh_list() (main.c): chica con pocos
            // puntos, crece con mas, tope ~210px con scroll interno mas
            // alla de eso — confirmado que este comportamiento es el que
            // se queria, no tocar de nuevo sin pedido explicito.
            lv_obj_t *left_col = lv_obj_create(body);
            lv_obj_set_size(left_col, LV_PCT(35), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(left_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(left_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(left_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(left_col, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(left_col, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_row(left_col, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *obj = lv_label_create(left_col);
                objects.ar_points_caption_label = obj;
                lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(obj, "PUNTOS GUARDADOS");
            }
            {
                // Lista de puntos — poblada/actualizada en runtime desde
                // main.c, que tambien ajusta el alto dinamicamente ahi
                // (ver auto_rotation_editor_refresh_list). 210 aca es solo
                // el valor inicial antes del primer refresh.
                lv_obj_t *obj = lv_list_create(left_col);
                objects.ar_points_list = obj;
                lv_obj_set_size(obj, LV_PCT(100), 210);
                lv_obj_set_style_bg_color(obj, lv_color_hex(0xff141414), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            }

            // ---- Columna derecha: velocidad, tarjetas de servo, acciones ----
            // LV_SIZE_CONTENT, mismo motivo que left_col: se estira justo
            // PCT(100) + SPACE_BETWEEN: en vez de adivinar cuantos px de
            // gap hacen falta para empujar "Test/Guardar" hacia abajo
            // (dos intentos seguidos calcularon mal — uno dejaba un marco
            // negro sin usar, el otro se pasaba y recortaba los botones
            // contra el borde fisico real), right_col ocupa el 100% real
            // del alto de "body" (sea cual sea) y SPACE_BETWEEN clava el
            // primer hijo (speed_card) arriba y el ultimo (actions_row)
            // exactamente abajo, repartiendo el resto como separacion —
            // sin depender de ningun numero fijo adivinado. Las tarjetas
            // se dejaron chicas (con margen de sobra) para que sobre
            // espacio real para repartir y esto no vuelva a recortar.
            lv_obj_t *right_col = lv_obj_create(body);
            lv_obj_set_size(right_col, LV_PCT(63), LV_PCT(100));
            lv_obj_set_style_bg_opa(right_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(right_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(right_col, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(right_col, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(right_col, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_main_place(right_col, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);
            {
                // Tarjeta Velocidad — fila unica, no suma profundidad de flex
                // (right_col > speed_card > 4 hijos = solo 1 nivel extra).
                lv_obj_t *speed_card = lv_obj_create(right_col);
                objects.ar_speed_card = speed_card;
                // Alto = LV_SIZE_CONTENT (en vez de un numero fijo adivinado):
                // la caja se ajusta exactamente al contenido + el padding
                // vertical explicito de abajo, asi no puede quedar mas alta
                // que el contenido con hueco desparejo arriba/abajo.
                lv_obj_set_size(speed_card, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_color(speed_card, lv_color_hex(0xff161616), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(speed_card, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(speed_card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(speed_card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_hor(speed_card, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_ver(speed_card, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(speed_card, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(speed_card, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_main_place(speed_card, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_cross_place(speed_card, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_column(speed_card, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(speed_card, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *obj = lv_label_create(speed_card);
                    objects.ar_speed_caption_label = obj;
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "VELOCIDAD");
                }
                lv_obj_t *speed_ctrl = lv_obj_create(speed_card);
                lv_obj_set_size(speed_ctrl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(speed_ctrl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(speed_ctrl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(speed_ctrl, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(speed_ctrl, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(speed_ctrl, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_cross_place(speed_ctrl, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_column(speed_ctrl, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(speed_ctrl, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *obj = lv_button_create(speed_ctrl);
                    objects.ar_speed_btn_decrease = obj;
                    lv_obj_set_size(obj, 46, 40);
                    lv_obj_add_event_cb(obj, ar_speed_dec_cb, LV_EVENT_PRESSED, (void *)0);
                    lv_obj_add_event_cb(obj, ar_speed_dec_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); }
                }
                {
                    lv_obj_t *obj = lv_label_create(speed_ctrl);
                    objects.ar_speed_label = obj;
                    lv_obj_set_size(obj, 90, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "150 ms");
                }
                {
                    lv_obj_t *obj = lv_button_create(speed_ctrl);
                    objects.ar_speed_btn_increase = obj;
                    lv_obj_set_size(obj, 46, 40);
                    lv_obj_add_event_cb(obj, ar_speed_inc_cb, LV_EVENT_PRESSED, (void *)0);
                    lv_obj_add_event_cb(obj, ar_speed_inc_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); }
                }

                // Fila de tarjetas Head/Neck — cada tarjeta es LV_LAYOUT_NONE
                // (hijos posicionados con lv_obj_set_pos): igual que en el
                // arreglo anterior, evita sumar niveles de flex anidado
                // (right_col > servo_row > tarjeta = 2 niveles, tarjeta misma
                // sin flex adentro) despues del crash de stack por anidar
                // demasiados flex-en-flex en esta pantalla.
                lv_obj_t *servo_row = lv_obj_create(right_col);
                // Alto = LV_SIZE_CONTENT: se ajusta al alto real de las
                // tarjetas (168, ver mas abajo) en vez de un numero fijo
                // aparte que podia terminar mas alto que el contenido real
                // (mismo ajuste que se hizo en speed_card).
                lv_obj_set_size(servo_row, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(servo_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(servo_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(servo_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(servo_row, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(servo_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_main_place(servo_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(servo_row, LV_OBJ_FLAG_SCROLLABLE);
                {
                    // HEAD · SERVO 1 — ancho 44%. Alto fijo 152 = margen
                    // superior (14) + contenido real (titulo+valor+botones,
                    // subidos a y=96 para no dejar un hueco enorme entre el
                    // valor "090°" y los botones +/-) + el mismo margen (14)
                    // espejado abajo. Los 168-152=16px que se liberan por
                    // tarjeta le quedan a right_col (SPACE_BETWEEN) para
                    // separar mas este bloque de SPEED arriba y de
                    // Test/Guardar abajo.
                    lv_obj_t *card = lv_obj_create(servo_row);
                    objects.ar_head_card = card;
                    lv_obj_set_size(card, LV_PCT(44), 152);
                    lv_obj_set_style_bg_color(card, lv_color_hex(0xff161616), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(card, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    // pad_all=0: sin esto el padding default (no cero) de
                    // lv_obj_create se suma al y absoluto de los hijos (LV_LAYOUT_NONE
                    // posiciona relativo al area de contenido, no al borde externo),
                    // corriendo todo hacia abajo y recortando los botones contra
                    // el borde inferior de la tarjeta.
                    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *obj = lv_label_create(card);
                        objects.ar_head_caption_label = obj;
                        lv_obj_set_pos(obj, 0, 14);
                        lv_obj_set_width(obj, LV_PCT(100));
                        lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        // "\xC2\xB7" (punto medio, ·) no esta en el subset de
                        // esta fuente montserrat_18 del firmware — se veia
                        // como un cuadrado/tofu en el equipo real. Guion en
                        // su lugar, que ya se confirmo que renderiza bien.
                        lv_label_set_text(obj, "HEAD - SERVO 1");
                    }
                    {
                        lv_obj_t *obj = lv_label_create(card);
                        objects.ar_angle_head_label = obj;
                        lv_obj_set_pos(obj, 0, 44);
                        lv_obj_set_width(obj, LV_PCT(100));
                        lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_label_set_text(obj, "090\xC2\xB0");
                    }
                    {
                        lv_obj_t *obj = lv_button_create(card);
                        objects.ar_angle_head_btn_decrese = obj;
                        lv_obj_set_pos(obj, 33, 96);
                        lv_obj_set_size(obj, 52, 42);
                        lv_obj_add_event_cb(obj, ar_angle_head_dec_cb, LV_EVENT_PRESSED, (void *)0);
                        lv_obj_add_event_cb(obj, ar_angle_head_dec_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                        lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); }
                    }
                    {
                        lv_obj_t *obj = lv_button_create(card);
                        objects.ar_angle_head_btn_increased = obj;
                        lv_obj_set_pos(obj, 101, 96);
                        lv_obj_set_size(obj, 52, 42);
                        lv_obj_add_event_cb(obj, ar_angle_head_inc_cb, LV_EVENT_PRESSED, (void *)0);
                        lv_obj_add_event_cb(obj, ar_angle_head_inc_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                        lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); }
                    }
                }
                {
                    // NECK · SERVO 2 — misma estructura que Head
                    lv_obj_t *card = lv_obj_create(servo_row);
                    objects.ar_neck_card = card;
                    lv_obj_set_size(card, LV_PCT(44), 152);
                    lv_obj_set_style_bg_color(card, lv_color_hex(0xff161616), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(card, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(card, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *obj = lv_label_create(card);
                        objects.ar_neck_caption_label = obj;
                        lv_obj_set_pos(obj, 0, 14);
                        lv_obj_set_width(obj, LV_PCT(100));
                        lv_obj_set_style_text_color(obj, lv_color_hex(0xff888888), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_label_set_text(obj, "NECK - SERVO 2");
                    }
                    {
                        lv_obj_t *obj = lv_label_create(card);
                        objects.ar_angle_neck_label = obj;
                        lv_obj_set_pos(obj, 0, 44);
                        lv_obj_set_width(obj, LV_PCT(100));
                        lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_label_set_text(obj, "090\xC2\xB0");
                    }
                    {
                        lv_obj_t *obj = lv_button_create(card);
                        objects.ar_angle_neck_btn_decrese = obj;
                        lv_obj_set_pos(obj, 33, 96);
                        lv_obj_set_size(obj, 52, 42);
                        lv_obj_add_event_cb(obj, ar_angle_neck_dec_cb, LV_EVENT_PRESSED, (void *)0);
                        lv_obj_add_event_cb(obj, ar_angle_neck_dec_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                        lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); }
                    }
                    {
                        lv_obj_t *obj = lv_button_create(card);
                        objects.ar_angle_neck_btn_increased = obj;
                        lv_obj_set_pos(obj, 101, 96);
                        lv_obj_set_size(obj, 52, 42);
                        lv_obj_add_event_cb(obj, ar_angle_neck_inc_cb, LV_EVENT_PRESSED, (void *)0);
                        lv_obj_add_event_cb(obj, ar_angle_neck_inc_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)0);
                        lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); }
                    }
                }

                // Fila de acciones: Test (outline) | Eliminar (outline, icono
                // solo, borra el punto tocado en la lista) | Guardar punto
                // (relleno). 3 botones en el mismo alto de siempre (52px),
                // Eliminar es angosto (18%) para no apretar los otros 2.
                lv_obj_t *actions_row = lv_obj_create(right_col);
                lv_obj_set_size(actions_row, LV_PCT(100), 46);
                lv_obj_set_style_bg_opa(actions_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(actions_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(actions_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(actions_row, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(actions_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_main_place(actions_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(actions_row, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *obj = lv_button_create(actions_row);
                    objects.ar_btn_probar = obj;
                    lv_obj_set_size(obj, LV_PCT(30), LV_PCT(100));
                    lv_obj_add_event_cb(obj, ar_btn_probar_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, LV_SYMBOL_PLAY " Test"); }
                }
                {
                    // Eliminar punto — borra el punto actualmente seleccionado
                    // en la lista (el que se toco para cargarlo, resaltado en
                    // amarillo oscuro). Si no hay ninguno tocado, no hace nada
                    // (ver ar_btn_eliminar_punto_cb en main.c).
                    lv_obj_t *obj = lv_button_create(actions_row);
                    objects.ar_btn_eliminar_punto = obj;
                    lv_obj_set_size(obj, LV_PCT(18), LV_PCT(100));
                    lv_obj_add_event_cb(obj, ar_btn_eliminar_punto_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_color(obj, lv_color_hex(0xffbc0f2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xffbc0f2d), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, LV_SYMBOL_TRASH); }
                }
                {
                    lv_obj_t *obj = lv_button_create(actions_row);
                    objects.ar_btn_guardar_punto = obj;
                    lv_obj_set_size(obj, LV_PCT(46), LV_PCT(100));
                    lv_obj_add_event_cb(obj, ar_btn_guardar_punto_cb, LV_EVENT_CLICKED, (void *)0);
                    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                    { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_color(lbl, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(lbl, LV_SYMBOL_SAVE " Guardar punto"); }
                }
            }
        }
    }
}

// Settings > Limites de Servo — grilla 2 filas (Minimo/Maximo) x 3 columnas
// (Servo 1/2/3), mismo patron nav+panel que Encoder/Tecnologia. Los valores
// se cargan/aplican en runtime desde main.c (hmi_srv_limits_load_to_ui /
// sl_limit_btn_cb / sl_btn_guardar_cb).
void create_panel_settings_srv_limits() {
    // ---- Boton de nav (inactivo) ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_btn_user);
        lv_obj_t *obj = lv_btn_create(parent_obj);
        objects.settings_btn_srv_limits = obj;
        lv_obj_set_size(obj, LV_PCT(100), 60);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(obj, action_settings_btn_srv_limits, LV_EVENT_CLICKED, (void *)0);
        {
            lv_obj_t *lbl = lv_label_create(obj);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(lbl, "Servo Limits");
        }
    }
    // ---- Panel (oculto por defecto) ----
    {
        lv_obj_t *parent_obj = lv_obj_get_parent(objects.settings_content_brightness);
        lv_obj_t *obj = lv_obj_create(parent_obj);
        objects.settings_content_srv_limits = obj;
        lv_obj_set_pos(obj, 0, 0);
        lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(obj, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        {
            lv_obj_t *parent_obj = obj;
            // Titulo (child 0)
            {
                lv_obj_t *obj = lv_label_create(parent_obj);
                lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_left(obj, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(obj, " SERVO LIMITS");
            }
            // Subtitulo (child 1)
            {
                lv_obj_t *obj = lv_label_create(parent_obj);
                lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
                lv_label_set_text(obj, "Rango de movimiento de cada servo (0-270 grados)");
            }
            // Encabezado de columnas (child 2)
            {
                lv_obj_t *obj = lv_obj_create(parent_obj);
                lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *parent_obj = obj;
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 70, LV_SIZE_CONTENT); lv_label_set_text(obj, ""); }
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                      lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(obj, "Servo 1"); }
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                      lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(obj, "Servo 2"); }
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                      lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(obj, "Servo 3"); }
                }
            }
            // Fila Minimo (child 3)
            {
                lv_obj_t *obj = lv_obj_create(parent_obj);
                lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *parent_obj = obj;
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 70, LV_SIZE_CONTENT);
                      lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(obj, "Min."); }
                    // Servo 1 min
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv1_min_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv1_min_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "000"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv1_min_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                    // Servo 2 min
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv2_min_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)100);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv2_min_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "000"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv2_min_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)101);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                    // Servo 3 min
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv3_min_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)200);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv3_min_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "000"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv3_min_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)201);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                }
            }
            // Fila Maximo (child 4)
            {
                lv_obj_t *obj = lv_obj_create(parent_obj);
                lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);
                lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                {
                    lv_obj_t *parent_obj = obj;
                    { lv_obj_t *obj = lv_label_create(parent_obj); lv_obj_set_size(obj, 70, LV_SIZE_CONTENT);
                      lv_obj_set_style_text_color(obj, lv_color_hex(0xffaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                      lv_label_set_text(obj, "Max."); }
                    // Servo 1 max
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv1_max_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)10);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv1_max_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "270"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv1_max_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)11);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                    // Servo 2 max
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv2_max_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)110);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv2_max_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "270"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv2_max_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)111);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                    // Servo 3 max
                    {
                        lv_obj_t *obj = lv_obj_create(parent_obj);
                        lv_obj_set_size(obj, 170, LV_SIZE_CONTENT);
                        lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_set_style_pad_column(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                        {
                            lv_obj_t *parent_obj = obj;
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv3_max_btn_decrese = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)210);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "-"); } }
                            { lv_obj_t *obj = lv_label_create(parent_obj); objects.sl_srv3_max_label = obj; lv_obj_set_size(obj, 55, LV_SIZE_CONTENT);
                              lv_obj_set_style_text_color(obj, lv_color_hex(0xfff5c518), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_label_set_text(obj, "270"); }
                            { lv_obj_t *obj = lv_button_create(parent_obj); objects.sl_srv3_max_btn_increased = obj; lv_obj_set_size(obj, 44, 40);
                              lv_obj_add_event_cb(obj, sl_limit_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)211);
                              lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                              lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                              { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); lv_label_set_text(lbl, "+"); } }
                        }
                    }
                }
            }
            // Guardar (child 5)
            {
                lv_obj_t *obj = lv_button_create(parent_obj);
                objects.sl_btn_guardar = obj;
                lv_obj_set_size(obj, 220, 55);
                lv_obj_add_event_cb(obj, sl_btn_guardar_cb, LV_EVENT_CLICKED, (void *)0);
                lv_obj_set_style_radius(obj, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2a2a2a), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(obj, lv_color_hex(0xff3c3c3c), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                { lv_obj_t *lbl = lv_label_create(obj); lv_obj_set_style_align(lbl, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
                  lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                  lv_label_set_text(lbl, "Guardar limites"); }
            }
        }
    }
}

// Overlay pantalla completa, oculto por defecto: se muestra durante la
// descarga OTA (backlight prendido, texto fijo, sin numeros que cambien)
// para dar feedback visual sin reintroducir el parpadeo por redibujados
// dinamicos. Se crea como ultimo hijo de objects.main para quedar siempre
// por encima del resto de la UI cuando se muestra.
void create_ota_progress_overlay() {
    lv_obj_t *obj = lv_obj_create(objects.main);
    objects.ota_progress_overlay = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    {
        // Splash del arranque (welltepp, 800x480) en vez del isotipo chico:
        // el isotipo (40x24) se ve borroso al agrandarlo porque no hay mas
        // detalle real que estirar. welltepp ya es del tamaño de pantalla,
        // sin escalar, asi que sale nitido.
        lv_obj_t *iso = lv_img_create(obj);
        lv_img_set_src(iso, &welltepp);
        lv_obj_center(iso);
    }
}

void tick_screen_main() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
}
