#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_robot_brightness_changed(lv_event_t * e);
extern void action_robot_brightness_released(lv_event_t * e);
extern void action_encoder_button_reset_clicked(lv_event_t * e);
extern void action_angle_neck_btn_decreased(lv_event_t * e);
extern void action_angle_neck_btn_increased(lv_event_t * e);
extern void action_angle_head_btn_decreased(lv_event_t * e);
extern void action_angle_head_btn_increased(lv_event_t * e);
extern void action_console_brightness_changed(lv_event_t * e);
extern void action_console_brightness_released(lv_event_t * e);
extern void action_btn_giro_automatico(lv_event_t * e);
extern void action_btn_start_demo(lv_event_t * e);
extern void action_btn_stop_demo(lv_event_t * e);
extern void action_btn_center(lv_event_t * e);
extern void action_angle_neck_btn_decreased_pressing(lv_event_t * e);
extern void action_angle_neck_btn_increased_pressing(lv_event_t * e);
extern void action_angle_head_btn_decreased_pressing(lv_event_t * e);
extern void action_angle_head_btn_increased_pressing(lv_event_t * e);
extern void action_angle_neck_label_pressed(lv_event_t * e);
extern void action_angle_head_label_pressed(lv_event_t * e);
extern void action_sysinfo_btn_device(lv_event_t *e);
extern void action_sysinfo_btn_version(lv_event_t *e);
extern void action_sysinfo_btn_logs(lv_event_t *e);
extern void action_sysinfo_btn_guide(lv_event_t *e);
extern void action_settings_btn_brightness(lv_event_t *e);
extern void action_settings_btn_theme(lv_event_t *e);
extern void action_settings_btn_battery(lv_event_t *e);
extern void action_settings_theme_dark(lv_event_t *e);
extern void action_settings_theme_classic(lv_event_t *e);
extern void action_settings_theme_light(lv_event_t *e);
extern void action_settings_bat_voltage(lv_event_t *e);
extern void action_settings_bat_percent(lv_event_t *e);
extern void action_settings_btn_language(lv_event_t *e);
extern void action_lang_es(lv_event_t *e);
extern void action_lang_en(lv_event_t *e);
extern void action_lang_pt(lv_event_t *e);
extern void action_settings_btn_user(lv_event_t *e);
extern void action_settings_user_edit_name(lv_event_t *e);
extern void action_settings_change_lock_pin(lv_event_t *e);
extern void action_settings_toggle_lock_pin(lv_event_t *e);
extern void action_settings_btn_encoder(lv_event_t *e);
extern void action_sysinfo_btn_update(lv_event_t *e);

/* Override en main.c: reaplica el color activo/inactivo del boton toggle de
 * WiFi (y el color del label de estado) cuando cambia el tema. */
extern void hmi_update_panel_retheme(void);

/* Theme initializer — call once after create_screens() */
extern void hmi_theme_apply(int theme);
extern int  hmi_theme_current(void);

/* Theme helpers for dynamic panels (e.g. DEV panel in main.c) */
extern void       hmi_style_btn(lv_obj_t *btn, bool active);
extern lv_color_t hmi_theme_accent(void);
extern lv_color_t hmi_theme_txt_primary(void);
extern lv_color_t hmi_theme_txt_secondary(void);
extern lv_color_t hmi_theme_bg_btn_inactive(void);
extern lv_color_t hmi_theme_bd_btn_inactive(void);
extern lv_color_t hmi_theme_txt_btn_inactive(void);
extern lv_color_t hmi_theme_bg_btn_active(void);
extern lv_color_t hmi_theme_bd_btn_active(void);
extern lv_color_t hmi_theme_txt_btn_active(void);
/* Override this to re-theme the DEV panel on theme change */
extern void hmi_dev_retheme(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/