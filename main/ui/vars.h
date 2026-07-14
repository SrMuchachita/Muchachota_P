#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_ENCODER = 0,
    FLOW_GLOBAL_VARIABLE_CONSOLE_BRIGHTNESS = 1,
    FLOW_GLOBAL_VARIABLE_ANGLE_NECK_VALUE = 2,
    FLOW_GLOBAL_VARIABLE_ANGLE_HEAD_VALUE = 3,
    FLOW_GLOBAL_VARIABLE_ANGLE_NECK_STRING = 4,
    FLOW_GLOBAL_VARIABLE_ANGLE_HEAD_STRING = 5,
    FLOW_GLOBAL_VARIABLE_BLUETOOTH_PASSWORD_STRING = 6
};

// Native global variables

extern const char *get_var_robot_voltage_percent();
extern void set_var_robot_voltage_percent(const char *value);
extern const char *get_var_console_voltage_percent();
extern void set_var_console_voltage_percent(const char *value);
extern const char *get_var_console_voltage();
extern void set_var_console_voltage(const char *value);
extern const char *get_var_robot_voltage();
extern void set_var_robot_voltage(const char *value);
extern const char *get_var_angle_x_value();
extern void set_var_angle_x_value(const char *value);
extern const char *get_var_angle_y_value();
extern void set_var_angle_y_value(const char *value);
extern const char *get_var_brightness();
extern void set_var_brightness(const char *value);
extern const char *get_var_encoder();
extern void set_var_encoder(const char *value);
extern const char *get_var_console_brightness();
extern void set_var_console_brightness(const char *value);
extern int32_t get_var_angle_neck_value();
extern void set_var_angle_neck_value(int32_t value);
extern int32_t get_var_angle_head_value();
extern void set_var_angle_head_value(int32_t value);
extern const char *get_var_angle_neck_string();
extern void set_var_angle_neck_string(const char *value);
extern const char *get_var_angle_head_string();
extern void set_var_angle_head_string(const char *value);
extern const char *get_var_bluetooth_password_string();
extern void set_var_bluetooth_password_string(const char *value);
extern const char *get_var_robot_serial();
extern void set_var_robot_serial(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/