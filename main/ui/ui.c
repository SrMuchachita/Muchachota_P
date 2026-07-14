#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include "main.h"

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

void ui_init() {
    create_screens();
    // Paneles ocultos por defecto (Encoder/Update/Theme/Battery/Language/
    // User/Version/Guide): se construyen aca, de un tiron, en el mismo lock
    // que create_screens(). Se probaron cuatro formas de diferir/escalonar
    // esta construccion (segundo lock, lv_timer, y lock+delay por etapas) y
    // las cuatro terminaron en pantalla negra trabada o congelada. Esta es
    // la unica arquitectura confirmada estable — no volver a tocar el
    // timing de arranque sin un log serial real que explique por que.
    create_panel_settings_encoder();
    create_panel_sysinfo_update();
    create_panel_settings_theme_battery_lang_user();
    create_panel_sysinfo_version_guide();
    enc_settings_load_from_nvs();
    hmi_theme_apply(1); /* Apply Classic theme to all widgets after screen creation */
    loadScreen(SCREEN_ID_MAIN);
}

void ui_tick() {
    tick_screen(currentScreen);
}