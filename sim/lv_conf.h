/*
 * lv_conf.h del SIMULADOR (sim/) — NO es el lv_conf real del firmware.
 * El firmware real configura LVGL via Kconfig/sdkconfig (ver CONFIG_LV_*
 * en el sdkconfig de la raiz). Este archivo es solo para el build de PC
 * dentro de sim/, y no afecta ni es usado por el build de ESP-IDF.
 *
 * Cualquier opcion no definida aca toma el default de lv_conf_internal.h.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16   // igual a CONFIG_LV_COLOR_DEPTH=16 del firmware real

#define LV_USE_SDL 1
#define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "SDL2/SDL.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (SDL_GetTicks())

#define LV_USE_OS LV_OS_NONE

// 256KB (el default chico pensado para el ESP32-P4) se quedaba corto en la
// PC: con todas las pantallas reales ya creadas (Ajustes, Sistema, MODOS,
// Panel de Manejo, columna del simulador) mas cualquier widget extra, el
// pool se fragmentaba/agotaba y el allocator interno de LVGL se ponia tan
// lento buscando espacio libre que Windows lo marcaba como "no responde"
// (reproducido: colgaba al agregar un boton mas). En la PC hay RAM de sobra,
// asi que le damos bastante margen.
#define LV_MEM_SIZE (16 * 1024U * 1024U)

// Fuentes Montserrat usadas por screens.c (ver fonts.h / dev_make_* / etc.)
#define LV_FONT_MONTSERRAT_8  1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_42 1
#define LV_FONT_MONTSERRAT_44 1
#define LV_FONT_MONTSERRAT_46 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1  // sin esto LV_USE_LOG no imprime nada (ver lv_conf_internal.h)
#endif

#endif /* LV_CONF_H */
