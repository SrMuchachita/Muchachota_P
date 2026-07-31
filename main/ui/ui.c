#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"
#include "main.h"
#include "lang.h"

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
    create_panel_settings_tecnologia();
    create_panel_modes_auto_rotation();
    create_panel_settings_srv_limits();
    create_ota_progress_overlay();
    enc_settings_load_from_nvs();
    hmi_auto_rotation_init(); /* carga NVS de recorridos/limites y arranca MODES en la vista home */
    // Preferencias de UI guardadas (tema/idioma/modo de bateria): se cargan
    // aca, antes de aplicar tema, para que un reinicio no vuelva siempre al
    // default de fabrica (Classic/ES/voltaje).
    lang_set((lang_id_t)hmi_ui_prefs_load_lang());
    lang_apply();
    g_bat_display_percent = (uint8_t)hmi_ui_prefs_load_bat_display();
    hmi_theme_apply(hmi_ui_prefs_load_theme()); /* aplica tema + reflejalo en los botones de bateria/idioma */
    loadScreen(SCREEN_ID_MAIN);
}

void ui_tick() {
    tick_screen(currentScreen);
}