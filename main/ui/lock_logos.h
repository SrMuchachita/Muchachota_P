#ifndef LOCK_LOGOS_H
#define LOCK_LOGOS_H

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

extern const lv_img_dsc_t lock_logo_wordmark; // Welltepp wordmark 150x24 (logotipo)
extern const lv_img_dsc_t lock_logo_isotipo;  // Welltepp isotipo  40x24 (marca W)

#endif // LOCK_LOGOS_H
