#include <string.h>
#include "vars.h"

/*console_brightnessr data*/
char console_brightness[10] = { 0 };

const char *get_var_console_brightness() {
    return console_brightness;
}

void set_var_console_brightness(const char *value) {
    strncpy(console_brightness, value, sizeof(console_brightness) / sizeof(char));
    console_brightness[sizeof(console_brightness) / sizeof(char) - 1] = '\0';
}

/*encoder data*/
char encoder[20] = { 0 };

const char *get_var_encoder() {
    return encoder;
}

void set_var_encoder(const char *value) {
    strncpy(encoder, value, sizeof(encoder) / sizeof(char));
    encoder[sizeof(encoder) / sizeof(char) - 1] = '\0';
}

/*console_voltage_percent data*/
char console_voltage_percent[10] = { 0 };

const char *get_var_console_voltage_percent() {
    return console_voltage_percent;
}

void set_var_console_voltage_percent(const char *value) {
    strncpy(console_voltage_percent, value, sizeof(console_voltage_percent) / sizeof(char));
    console_voltage_percent[sizeof(console_voltage_percent) / sizeof(char) - 1] = '\0';
}

/*robot_voltage_percent data*/
char robot_voltage_percent[10] = { 0 };

const char *get_var_robot_voltage_percent() {
    return robot_voltage_percent;
}

void set_var_robot_voltage_percent(const char *value) {
    strncpy(robot_voltage_percent, value, sizeof(robot_voltage_percent) / sizeof(char));
    robot_voltage_percent[sizeof(robot_voltage_percent) / sizeof(char) - 1] = '\0';
}

/*Brightness data*/
char brightness[10] = { 0 };

const char *get_var_brightness() {
    return brightness;
}

void set_var_brightness(const char *value) {
    strncpy(brightness, value, sizeof(brightness) / sizeof(char));
    brightness[sizeof(brightness) / sizeof(char) - 1] = '\0';
}

/*angle_x_value data*/
char angle_x_value[10] = { 0 };

const char *get_var_angle_x_value() {
    return angle_x_value;
}

void set_var_angle_x_value(const char *value) {
    strncpy(angle_x_value, value, sizeof(angle_x_value) / sizeof(char));
    angle_x_value[sizeof(angle_x_value) / sizeof(char) - 1] = '\0';
}

/*angle_y_value data*/
char angle_y_value[10] = { 0 };

const char *get_var_angle_y_value() {
    return angle_y_value;
}

void set_var_angle_y_value(const char *value) {
    strncpy(angle_y_value, value, sizeof(angle_y_value) / sizeof(char));
    angle_y_value[sizeof(angle_y_value) / sizeof(char) - 1] = '\0';
}

/*console_voltage data*/
char console_voltage[10] = { 0 };

const char *get_var_console_voltage() {
    return console_voltage;
}

void set_var_console_voltage(const char *value) {
    strncpy(console_voltage, value, sizeof(console_voltage) / sizeof(char));
    console_voltage[sizeof(console_voltage) / sizeof(char) - 1] = '\0';
}

/*robot_voltage data*/
char robot_voltage[10] = { 0 };

const char *get_var_robot_voltage() {
    return robot_voltage;
}

void set_var_robot_voltage(const char *value) {
    strncpy(robot_voltage, value, sizeof(robot_voltage) / sizeof(char));
    robot_voltage[sizeof(robot_voltage) / sizeof(char) - 1] = '\0';
}

/*angle_neck_value data*/
int32_t angle_neck_value = 90;

int32_t get_var_angle_neck_value() {
    return angle_neck_value;
}

void set_var_angle_neck_value(int32_t value) {
    angle_neck_value = value;
}

/*angle_head_value data*/
int32_t angle_head_value = 90;

int32_t get_var_angle_head_value() {
    return angle_head_value;
}

void set_var_angle_head_value(int32_t value) {
    angle_head_value = value;
}

/*angle_neck_string data*/
char angle_neck_string[10] = { 0 };

const char *get_var_angle_neck_string() {
    return angle_neck_string;
}

void set_var_angle_neck_string(const char *value) {
    strncpy(angle_neck_string, value, sizeof(angle_neck_string) / sizeof(char));
    angle_neck_string[sizeof(angle_neck_string) / sizeof(char) - 1] = '\0';
}

/*angle_head_string data*/
char angle_head_string[10] = { 0 };

const char *get_var_angle_head_string() {
    return angle_head_string;
}

void set_var_angle_head_string(const char *value) {
    strncpy(angle_head_string, value, sizeof(angle_head_string) / sizeof(char));
    angle_head_string[sizeof(angle_head_string) / sizeof(char) - 1] = '\0';
}

/*robot_serial data*/
char robot_serial[30] = { 0 };

const char *get_var_robot_serial() {
    return robot_serial;
}

void set_var_robot_serial(const char *value) {
    strncpy(robot_serial, value, sizeof(robot_serial) / sizeof(char));
    robot_serial[sizeof(robot_serial) / sizeof(char) - 1] = '\0';
}

/*bluetooth_passwor_string*/
char bluetooth_password_string[20] = { 0 };

const char *get_var_bluetooth_password_string() {
    return bluetooth_password_string;
}

void set_var_bluetooth_password_string(const char *value) {
    strncpy(bluetooth_password_string, value, sizeof(bluetooth_password_string) / sizeof(char));
    bluetooth_password_string[sizeof(bluetooth_password_string) / sizeof(char) - 1] = '\0';
}