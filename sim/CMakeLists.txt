# Build de PC del simulador de UI (LVGL + SDL2). Totalmente independiente
# del build de ESP-IDF (idf.py) del resto del proyecto — no lo reemplaza,
# no lo toca, no comparte configuracion con el.
#
# Uso:
#   cmake -S sim -B sim/build
#   cmake --build sim/build
#   sim/build/sim   (o sim/build/Debug/sim.exe con el generador de Visual Studio)

cmake_minimum_required(VERSION 3.16)
project(wp_p_v9_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL2 vendorizado dentro de sim/ (no depende de find_package ni de rutas
# globales de Scoop/vcpkg). El .lib que trae el paquete "SDL2" de Scoop es
# formato MSVC y no linkea con MinGW, asi que se genero un import library
# compatible (libSDL2.dll.a) a partir del SDL2.dll real con gendef+dlltool.
set(SDL2_ROOT        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl2)
set(SDL2_INCLUDE_DIR ${SDL2_ROOT}/include)
set(SDL2_LIB_DIR     ${SDL2_ROOT}/lib)

set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl)
set(UI_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/../main/ui)

file(GLOB_RECURSE LVGL_SOURCES ${LVGL_DIR}/src/*.c)

add_executable(sim
    sim_main.c
    stubs/main_stub.c
    stubs/bigview_port.c   # Panel de Manejo (logo WELLTEP) — port real, ver el archivo
    stubs/modes_port.c     # MODOS (Control por Puntos / Giro Automatico) — port real, ver el archivo
    stubs/dev_mode_port.c  # MODO DESARROLLADOR (triple-toque en Sistema > Version) — port real, ver el archivo
    stubs/modbus_rtu.c     # cliente Modbus RTU sobre COM (para el robot real por USB-RS485)
    stubs/robot_panel.c    # panel "ROBOT REAL (RS485)" — solo simulador, ver el archivo
    stubs/sim_log.c        # panel de LOG (usa el espacio de sobra al maximizar) — solo simulador
    stubs/stb_image_write_impl.c  # implementacion de stb_image_write (guarda las capturas en PNG)
    ${UI_DIR}/screens.c
    ${UI_DIR}/actions.c
    ${UI_DIR}/ui.c
    ${UI_DIR}/vars.c
    ${UI_DIR}/lang.c
    ${UI_DIR}/styles.c
    ${UI_DIR}/images.c
    ${UI_DIR}/game_shooter.c    # juego COSMO DEFENSA (prueba en simulador)
    ${UI_DIR}/game_platform.c   # juego de plataformas + disparos (base de prueba)
    ${UI_DIR}/lock_logos.c
    ${UI_DIR}/font_distance.c   # fuente grande del numero del Panel de Manejo
    ${LVGL_SOURCES}
)

target_include_directories(sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}         # nuestro lv_conf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs   # nuestro main.h falso (el real de main/ NO se agrega aca)
    ${LVGL_DIR}
    ${LVGL_DIR}/src
    ${UI_DIR}
    ${SDL2_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb  # stb_image_write.h (capturas en PNG)
)

target_compile_definitions(sim PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_KCONFIG_IGNORE   # el lvgl de managed_components/ esta armado para Kconfig de ESP-IDF; esto lo desactiva
    SDL_MAIN_HANDLED
)

target_link_directories(sim PRIVATE ${SDL2_LIB_DIR})
target_link_libraries(sim PRIVATE SDL2 advapi32)
# SDL_MAIN_HANDLED + main() propio en sim_main.c: no hace falta SDL2main.

# Copiar SDL2.dll junto al .exe generado, para poder correrlo directo.
add_custom_command(TARGET sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_LIB_DIR}/SDL2.dll $<TARGET_FILE_DIR:sim>
)
