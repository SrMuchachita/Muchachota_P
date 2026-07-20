#include <stdio.h>
#include "lang.h"
#include "screens.h"

/* ---- Spanish ---- */
static const lang_strings_t s_es = {
    .tab_general        = "CONTROL GENERAL",
    .tab_modes          = "MODOS",
    .tab_settings       = "AJUSTES",
    .tab_sysinfo        = "SISTEMA",
    .sec_battery        = " BATERIA SISTEMA",
    .sec_tilt           = " INCLINACION",
    .sec_led_brightness = " BRILLO LED ROBOT",
    .sec_encoder        = " ENCODER",
    .lbl_robot_voltage  = "Voltaje Robot",
    .lbl_console_voltage= "Voltaje Consola",
    .lbl_angle_x        = "Angulo X",
    .lbl_angle_y        = "Angulo Y",
    .sec_neck           = " ANGULO CUELLO",
    .sec_head           = " ANGULO CABEZA",
    .btn_auto_rotation  = "AUTO\nROTACION",
    .btn_start_demo     = "INICIAR DEMO",
    .btn_stop_demo      = "DETENER DEMO",
    .btn_center         = "CENTRAR",
    .btn_reset          = "RESET",
    .nav_brightness     = "Brillo",
    .nav_theme          = "Tema",
    .nav_battery        = "Bateria",
    .nav_language       = "Idioma",
    .title_brightness   = " BRILLO",
    .title_appearance   = " APARIENCIA",
    .title_battery_display = " DISPLAY BATERIA",
    .title_language     = " IDIOMA",
    .lbl_screen         = "Pantalla :",
    .btn_dark           = "Modo Oscuro",
    .btn_classic        = "Clasico",
    .btn_light          = "Modo Claro",
    .btn_voltage        = "Voltaje (V)",
    .btn_percent        = "Porcentaje (%)",
    .nav_device         = "Dispositivo",
    .nav_version        = "Version",
    .nav_logs           = "Registros",
    .nav_guide          = "Guia",
    .title_device_info       = " INFO DISPOSITIVO",
    .title_version_info      = " INFO VERSION",
    .title_system_logs       = " REGISTROS",
    .title_quick_guide       = " GUIA RAPIDA",
    .guide_body              =
        "Controles Generales:\n"
        "  Monitorea bateria, angulos de inclinacion, encoder y brillo LED.\n\n"
        "Modos:\n"
        "  Ajusta angulos de cabeza/cuello, giro automatico o secuencias demo.\n\n"
        "Ajustes:\n"
        "  Configura brillo de pantalla, tema visual e idioma.\n\n"
        "Info Sistema:\n"
        "  Consulta detalles del dispositivo, version de firmware, registros y esta guia.",
    .alert_battery_critical  = "Bateria\ncritica",
    .alert_shutdown          = "APAGAR",
    .nav_user                = "Usuario",
    .title_user              = " USUARIO",
    .lbl_name                = "Nombre :",
    .btn_change_name         = "Cambiar Nombre",
    .btn_change_pin          = "Cambiar PIN",
    .lbl_enable_password     = "Habilitar contrasena",
    .title_device_name_editor= "NOMBRE DEL DISPOSITIVO",
    .btn_save                = "Guardar",
    .btn_cancel              = "Cancelar",
    .title_change_pin        = "CAMBIAR PIN",
    .title_dev_mode          = "MODO DESARROLLADOR",
    .sub_change_pin          = "Ingresa el nuevo PIN de 4 digitos",
    .sub_enter_pin           = "Ingresa el PIN para continuar",
    .nav_encoder             = "Encoder",
    .title_encoder           = " ENCODER",
    .desc_encoder            = "Perimetro del rodillo del encoder. Formula: Dist.(m) = Pulsos x Perimetro / 1000  |  Default: 85.20 mm",
    .lbl_perimeter           = "Perimetro:",
    .lbl_pulses_rev          = "Pulsos/vuelta:",
    .unit_pulses_rev         = "pulsos/vuelta",
    .fmt_encoder_footer      = "1 pulso = %.3f mm  \xC2\xB7  1 m = %.1f pulsos  \xC2\xB7  Formula: dist = pulsos x %.2f / (%ld x 1000)",
    .nav_technology          = "Tecnologia",
    .title_technology        = " TECNOLOGIA",
    .desc_technology         = "Tecnologia de video",
    .desc_tech_a             = "A: Cambio de senal de salida",
    .desc_tech_b             = "B: Resolucion de salida de senal o cambio de modo de TV",
    .nav_update              = "Actualizar",
    .title_update            = " ACTUALIZAR",
    .lbl_wifi_off            = "WiFi apagado",
    .lbl_connecting          = "Conectando...",
    .lbl_reconnecting        = "Reconectando...",
    .lbl_connected_prefix    = "Conectado: ",
    .btn_enable_wifi         = "Activar WiFi",
    .btn_disable_wifi        = "Desactivar WiFi",
    .lbl_updating            = "Actualizando...",
    .lbl_network_prefix      = "Red: ",
    .lbl_update_duration_note = "La actualizacion puede demorar aproximadamente 35 segundos",
    .btn_pulses              = "PULSOS",
    .btn_meters              = "METROS",
};

/* ---- English ---- */
static const lang_strings_t s_en = {
    .tab_general        = "GENERAL CONTROLS",
    .tab_modes          = "MODES",
    .tab_settings       = "SETTINGS",
    .tab_sysinfo        = "SYSTEM INFO",
    .sec_battery        = " SYSTEM BATTERY",
    .sec_tilt           = " TILT",
    .sec_led_brightness = " ROBOT LED BRIGHTNESS",
    .sec_encoder        = " ENCODER",
    .lbl_robot_voltage  = "Robot Voltage",
    .lbl_console_voltage= "Console Voltage",
    .lbl_angle_x        = "Angle X",
    .lbl_angle_y        = "Angle Y",
    .sec_neck           = " NECK START ANGLE",
    .sec_head           = " HEAD START ANGLE",
    .btn_auto_rotation  = "START AUTO\nROTATION",
    .btn_start_demo     = "START DEMO",
    .btn_stop_demo      = "STOP DEMO",
    .btn_center         = "CENTER",
    .btn_reset          = "RESET",
    .nav_brightness     = "Brightness",
    .nav_theme          = "Theme",
    .nav_battery        = "Battery",
    .nav_language       = "Language",
    .title_brightness   = " BRIGHTNESS",
    .title_appearance   = " APPEARANCE",
    .title_battery_display = " BATTERY DISPLAY",
    .title_language     = " LANGUAGE",
    .lbl_screen         = "Screen :",
    .btn_dark           = "Dark Mode",
    .btn_classic        = "Classic",
    .btn_light          = "Light Mode",
    .btn_voltage        = "Voltage (V)",
    .btn_percent        = "Percent (%)",
    .nav_device         = "Device",
    .nav_version        = "Version",
    .nav_logs           = "Logs",
    .nav_guide          = "Guide",
    .title_device_info       = " DEVICE INFORMATION",
    .title_version_info      = " VERSION INFO",
    .title_system_logs       = " SYSTEM LOGS",
    .title_quick_guide       = " QUICK GUIDE",
    .guide_body              =
        "General Controls:\n"
        "  Monitor battery, tilt angles, encoder and LED brightness.\n\n"
        "Modes:\n"
        "  Set head/neck angles, run auto-rotation or demo sequences.\n\n"
        "Settings:\n"
        "  Configure display brightness, visual theme and language.\n\n"
        "System Info:\n"
        "  View device details, firmware version, logs and this guide.",
    .alert_battery_critical  = "Battery\ncritical",
    .alert_shutdown          = "SHUTDOWN",
    .nav_user                = "User",
    .title_user              = " USER",
    .lbl_name                = "Name :",
    .btn_change_name         = "Change Name",
    .btn_change_pin          = "Change PIN",
    .lbl_enable_password     = "Enable password",
    .title_device_name_editor= "DEVICE NAME",
    .btn_save                = "Save",
    .btn_cancel              = "Cancel",
    .title_change_pin        = "CHANGE PIN",
    .title_dev_mode          = "DEVELOPER MODE",
    .sub_change_pin          = "Enter the new 4-digit PIN",
    .sub_enter_pin           = "Enter the PIN to continue",
    .nav_encoder             = "Encoder",
    .title_encoder           = " ENCODER",
    .desc_encoder            = "Encoder roller perimeter. Formula: Dist.(m) = Pulses x Perimeter / 1000  |  Default: 85.20 mm",
    .lbl_perimeter           = "Perimeter:",
    .lbl_pulses_rev          = "Pulses/rev:",
    .unit_pulses_rev         = "pulses/rev",
    .fmt_encoder_footer      = "1 pulse = %.3f mm  \xC2\xB7  1 m = %.1f pulses  \xC2\xB7  Formula: dist = pulses x %.2f / (%ld x 1000)",
    .nav_technology          = "Technology",
    .title_technology        = " TECHNOLOGY",
    .desc_technology         = "Video technology",
    .desc_tech_a             = "A: Output signal change",
    .desc_tech_b             = "B: Output signal resolution or TV mode change",
    .nav_update              = "Update",
    .title_update            = " UPDATE",
    .lbl_wifi_off            = "WiFi off",
    .lbl_connecting          = "Connecting...",
    .lbl_reconnecting        = "Reconnecting...",
    .lbl_connected_prefix    = "Connected: ",
    .btn_enable_wifi         = "Enable WiFi",
    .btn_disable_wifi        = "Disable WiFi",
    .lbl_updating            = "Updating...",
    .lbl_network_prefix      = "Network: ",
    .lbl_update_duration_note = "The update may take approximately 35 seconds",
    .btn_pulses              = "PULSES",
    .btn_meters              = "METERS",
};

/* ---- Portuguese ---- */
static const lang_strings_t s_pt = {
    .tab_general        = "CONTROLES GERAIS",
    .tab_modes          = "MODOS",
    .tab_settings       = "CONFIGURACOES",
    .tab_sysinfo        = "INFO SISTEMA",
    .sec_battery        = " BATERIA SISTEMA",
    .sec_tilt           = " INCLINACAO",
    .sec_led_brightness = " BRILHO LED ROBO",
    .sec_encoder        = " ENCODER",
    .lbl_robot_voltage  = "Tensao Robo",
    .lbl_console_voltage= "Tensao Console",
    .lbl_angle_x        = "Angulo X",
    .lbl_angle_y        = "Angulo Y",
    .sec_neck           = " ANGULO PESCOCO",
    .sec_head           = " ANGULO CABECA",
    .btn_auto_rotation  = "AUTO\nROTACAO",
    .btn_start_demo     = "INICIAR DEMO",
    .btn_stop_demo      = "PARAR DEMO",
    .btn_center         = "CENTRALIZAR",
    .btn_reset          = "RESET",
    .nav_brightness     = "Brilho",
    .nav_theme          = "Tema",
    .nav_battery        = "Bateria",
    .nav_language       = "Idioma",
    .title_brightness   = " BRILHO",
    .title_appearance   = " APARENCIA",
    .title_battery_display = " DISPLAY BATERIA",
    .title_language     = " IDIOMA",
    .lbl_screen         = "Tela :",
    .btn_dark           = "Modo Escuro",
    .btn_classic        = "Classico",
    .btn_light          = "Modo Claro",
    .btn_voltage        = "Tensao (V)",
    .btn_percent        = "Porcentagem (%)",
    .nav_device         = "Dispositivo",
    .nav_version        = "Versao",
    .nav_logs           = "Registros",
    .nav_guide          = "Guia",
    .title_device_info       = " INFO DISPOSITIVO",
    .title_version_info      = " INFO VERSAO",
    .title_system_logs       = " LOGS SISTEMA",
    .title_quick_guide       = " GUIA RAPIDA",
    .guide_body              =
        "Controles Gerais:\n"
        "  Monitore bateria, angulos de inclinacao, encoder e brilho LED.\n\n"
        "Modos:\n"
        "  Ajuste angulos de cabeca/pescoco, rotacao automatica ou sequencias demo.\n\n"
        "Configuracoes:\n"
        "  Configure brilho da tela, tema visual e idioma.\n\n"
        "Info Sistema:\n"
        "  Veja detalhes do dispositivo, versao do firmware, logs e este guia.",
    .alert_battery_critical  = "Bateria\ncritica",
    .alert_shutdown          = "DESLIGAR",
    .nav_user                = "Usuario",
    .title_user              = " USUARIO",
    .lbl_name                = "Nome :",
    .btn_change_name         = "Alterar Nome",
    .btn_change_pin          = "Alterar PIN",
    .lbl_enable_password     = "Habilitar senha",
    .title_device_name_editor= "NOME DO DISPOSITIVO",
    .btn_save                = "Salvar",
    .btn_cancel              = "Cancelar",
    .title_change_pin        = "ALTERAR PIN",
    .title_dev_mode          = "MODO DESENVOLVEDOR",
    .sub_change_pin          = "Digite o novo PIN de 4 digitos",
    .sub_enter_pin           = "Digite o PIN para continuar",
    .nav_encoder             = "Encoder",
    .title_encoder           = " ENCODER",
    .desc_encoder            = "Perimetro do rolete do encoder. Formula: Dist.(m) = Pulsos x Perimetro / 1000  |  Padrao: 85.20 mm",
    .lbl_perimeter           = "Perimetro:",
    .lbl_pulses_rev          = "Pulsos/volta:",
    .unit_pulses_rev         = "pulsos/volta",
    .fmt_encoder_footer      = "1 pulso = %.3f mm  \xC2\xB7  1 m = %.1f pulsos  \xC2\xB7  Formula: dist = pulsos x %.2f / (%ld x 1000)",
    .nav_technology          = "Tecnologia",
    .title_technology        = " TECNOLOGIA",
    .desc_technology         = "Tecnologia de video",
    .desc_tech_a             = "A: Mudanca de sinal de saida",
    .desc_tech_b             = "B: Resolucao de saida de sinal ou mudanca de modo de TV",
    .nav_update              = "Atualizar",
    .title_update            = " ATUALIZAR",
    .lbl_wifi_off            = "WiFi desligado",
    .lbl_connecting          = "Conectando...",
    .lbl_reconnecting        = "Reconectando...",
    .lbl_connected_prefix    = "Conectado: ",
    .btn_enable_wifi         = "Ativar WiFi",
    .btn_disable_wifi        = "Desativar WiFi",
    .lbl_updating            = "Atualizando...",
    .lbl_network_prefix      = "Rede: ",
    .lbl_update_duration_note = "A atualizacao pode demorar aproximadamente 35 segundos",
    .btn_pulses              = "PULSOS",
    .btn_meters              = "METROS",
};

/* ---- Runtime state ---- */
static const lang_strings_t *s_table[LANG_COUNT] = { &s_es, &s_en, &s_pt };
static lang_id_t             s_id  = LANG_EN;
const  lang_strings_t       *g_lang = &s_en;

void lang_set(lang_id_t id) {
    if ((unsigned)id >= LANG_COUNT) id = LANG_EN;
    s_id   = id;
    g_lang = s_table[id];
}

lang_id_t lang_get(void) { return s_id; }

/* ---- Apply current language to all UI labels ---- */
#define LBL(obj, txt)      lv_label_set_text((obj), (txt))
#define CLBL(obj, txt)     lv_label_set_text(lv_obj_get_child((obj), 0), (txt))

void lang_apply(void) {
    const lang_strings_t *L = g_lang;
    if (!L) return;

    /* Tab bar */
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(objects.tabview);
    const char *tabs[4] = { L->tab_general, L->tab_modes, L->tab_settings, L->tab_sysinfo };
    uint32_t n = lv_obj_get_child_count(tab_bar);
    for (uint32_t i = 0; i < n && i < 4; i++) {
        lv_obj_t *lbl = lv_obj_get_child(lv_obj_get_child(tab_bar, i), 0);
        if (lbl) lv_label_set_text(lbl, tabs[i]);
    }

    /* General Controls - section titles (labels with left border) */
    LBL(objects.obj7,  L->sec_battery);
    LBL(objects.obj11, L->sec_tilt);
    LBL(objects.obj15, L->sec_led_brightness);
    LBL(objects.obj18, L->sec_encoder);

    /* General Controls - field labels */
    LBL(objects.obj8,  L->lbl_robot_voltage);
    LBL(objects.obj9,  L->lbl_console_voltage);
    LBL(objects.obj12, L->lbl_angle_x);
    LBL(objects.obj13, L->lbl_angle_y);

    /* Modes - section titles */
    LBL(objects.obj20, L->sec_neck);
    LBL(objects.obj22, L->sec_head);

    /* Modes - action button labels (obj23/24/25 are the label objects directly) */
    LBL(objects.obj23, L->btn_auto_rotation);
    LBL(objects.obj24, L->btn_start_demo);
    LBL(objects.obj25, L->btn_stop_demo);
    CLBL(objects.btn_center,        L->btn_center);
    CLBL(objects.btn_encoder_reset, L->btn_reset);

    /* Settings - nav buttons */
    CLBL(objects.settings_btn_brightness, L->nav_brightness);
    CLBL(objects.settings_btn_theme,      L->nav_theme);
    CLBL(objects.settings_btn_battery,    L->nav_battery);
    CLBL(objects.settings_btn_language,   L->nav_language);

    /* Settings - content panel titles (child 0 of each panel) */
    CLBL(objects.settings_content_brightness, L->title_brightness);
    CLBL(objects.settings_content_theme,      L->title_appearance);
    CLBL(objects.settings_content_battery,    L->title_battery_display);
    CLBL(objects.settings_content_language,   L->title_language);

    /* Settings - Brightness: "Screen :" label is child 0 of obj28 (slider row) */
    CLBL(objects.obj28, L->lbl_screen);

    /* Settings - Theme buttons */
    CLBL(objects.settings_btn_theme_dark,    L->btn_dark);
    CLBL(objects.settings_btn_theme_classic, L->btn_classic);
    CLBL(objects.settings_btn_theme_light,   L->btn_light);

    /* Settings - Battery buttons */
    CLBL(objects.settings_btn_bat_voltage, L->btn_voltage);
    CLBL(objects.settings_btn_bat_percent, L->btn_percent);

    /* Sysinfo - nav buttons */
    CLBL(objects.sysinfo_btn_device,  L->nav_device);
    CLBL(objects.sysinfo_btn_version, L->nav_version);
    CLBL(objects.sysinfo_btn_guide,   L->nav_guide);

    /* Sysinfo - content panel titles (child 0 of each panel) */
    CLBL(objects.sysinfo_content_device,  L->title_device_info);
    CLBL(objects.sysinfo_content_version, L->title_version_info);
    CLBL(objects.sysinfo_content_guide,   L->title_quick_guide);

    /* Sysinfo - guide body text (child 1 of guide panel) */
    {
        lv_obj_t *guide_body = lv_obj_get_child(objects.sysinfo_content_guide, 1);
        if (guide_body) lv_label_set_text(guide_body, L->guide_body);
    }

    /* Settings - User nav button */
    CLBL(objects.settings_btn_user, L->nav_user);

    /* Settings - User panel (child order: 0=title, 1=name row, 2=buttons row, 3=password row) */
    if (objects.settings_content_user) {
        lv_obj_t *panel = objects.settings_content_user;
        lv_obj_t *title = lv_obj_get_child(panel, 0);
        if (title) lv_label_set_text(title, L->title_user);

        lv_obj_t *name_row = lv_obj_get_child(panel, 1);
        if (name_row) {
            lv_obj_t *name_lbl = lv_obj_get_child(name_row, 0);
            if (name_lbl) lv_label_set_text(name_lbl, L->lbl_name);
        }

        lv_obj_t *btn_row = lv_obj_get_child(panel, 2);
        if (btn_row) {
            lv_obj_t *btn_name = lv_obj_get_child(btn_row, 0);
            if (btn_name) CLBL(btn_name, L->btn_change_name);
            lv_obj_t *btn_pin = lv_obj_get_child(btn_row, 1);
            if (btn_pin) CLBL(btn_pin, L->btn_change_pin);
        }

        lv_obj_t *pw_row = lv_obj_get_child(panel, 3);
        if (pw_row) {
            lv_obj_t *pw_lbl = lv_obj_get_child(pw_row, 0);
            if (pw_lbl) lv_label_set_text(pw_lbl, L->lbl_enable_password);
        }
    }

    /* Settings - Encoder nav button */
    CLBL(objects.settings_btn_encoder, L->nav_encoder);

    /* Settings - Encoder panel (child order: 0=title,1=desc,2=perimeter row,
     * 3=presets,4=pulses/rev row,5=footer). El footer se recalcula del lado
     * de main.c (formula con numeros), no aca. */
    if (objects.settings_content_encoder) {
        lv_obj_t *panel = objects.settings_content_encoder;
        lv_obj_t *title = lv_obj_get_child(panel, 0);
        if (title) lv_label_set_text(title, L->title_encoder);
        lv_obj_t *desc = lv_obj_get_child(panel, 1);
        if (desc) lv_label_set_text(desc, L->desc_encoder);
        lv_obj_t *perim_row = lv_obj_get_child(panel, 2);
        if (perim_row) {
            lv_obj_t *key = lv_obj_get_child(perim_row, 0);
            if (key) lv_label_set_text(key, L->lbl_perimeter);
        }
        lv_obj_t *ppr_row = lv_obj_get_child(panel, 4);
        if (ppr_row) {
            lv_obj_t *key = lv_obj_get_child(ppr_row, 0);
            if (key) lv_label_set_text(key, L->lbl_pulses_rev);
            lv_obj_t *unit = lv_obj_get_child(ppr_row, 2);
            if (unit) lv_label_set_text(unit, L->unit_pulses_rev);
        }
    }

    /* Settings - Technology nav button + panel (0=title, 1=desc, 2=fila
     * botones A/B, 3=desc A, 4=desc B) */
    CLBL(objects.settings_btn_tecnologia, L->nav_technology);
    if (objects.settings_content_tecnologia) {
        lv_obj_t *panel = objects.settings_content_tecnologia;
        lv_obj_t *title = lv_obj_get_child(panel, 0);
        if (title) lv_label_set_text(title, L->title_technology);
        lv_obj_t *desc = lv_obj_get_child(panel, 1);
        if (desc) lv_label_set_text(desc, L->desc_technology);
    }
    if (objects.tech_desc_a) lv_label_set_text(objects.tech_desc_a, L->desc_tech_a);
    if (objects.tech_desc_b) lv_label_set_text(objects.tech_desc_b, L->desc_tech_b);

    /* Sysinfo - Update nav button + panel (0=title,1=LED+status,2=button
     * row,3=network info). El texto del boton WiFi y el estado dependen de
     * estado que solo main.c conoce — ver hmi_extra_panels_apply_lang(). */
    CLBL(objects.sysinfo_btn_update, L->nav_update);
    if (objects.sysinfo_content_update) {
        lv_obj_t *panel = objects.sysinfo_content_update;
        lv_obj_t *title = lv_obj_get_child(panel, 0);
        if (title) lv_label_set_text(title, L->title_update);
        if (objects.update_network_label) {
            char buf[48];
            snprintf(buf, sizeof(buf), "%sWTP TALLER", L->lbl_network_prefix);
            lv_label_set_text(objects.update_network_label, buf);
        }
    }
    if (objects.update_duration_note) lv_label_set_text(objects.update_duration_note, L->lbl_update_duration_note);

    /* Overlay "Actualizando..." de la descarga OTA (reusa lbl_updating) */
    if (objects.ota_progress_label) lv_label_set_text(objects.ota_progress_label, L->lbl_updating);

    hmi_extra_panels_apply_lang();
}

#undef LBL
#undef CLBL
