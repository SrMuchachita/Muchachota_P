/*
 * Paleta de colores para los paneles PROPIOS del simulador — columnas
 * MOTOR/SERVOS, el joystick, el LOG y los controles "solo simulador"
 * (Simular datos, Capturar, Modelo robot, Ping/Pong).
 *
 * IMPORTANTE: esto NO toca el HMI real (screens.c/ui.c) — esos colores son
 * el diseño real de la pantalla y quedan intactos.
 *
 * A proposito casi monocromo, como VS Code (Dark+): fondo casi negro, gris
 * neutro para todo lo que no esta "activo", y UN SOLO acento azul para todo
 * lo interactivo/resaltado (bordes de foco, valores, boton primario) — nada
 * de verde/rojo/naranja/violeta salteados por estado o por columna.
 */

#ifndef SIM_THEME_H
#define SIM_THEME_H

#define SIM_BG        0xff1e1e1e // fondo general (VS Code editor.background)
#define SIM_PANEL     0xff252526 // fondo de tarjetas (VS Code sideBar.background)
#define SIM_BORDER    0xff3c3c3c // bordes/separadores sutiles
#define SIM_TEXT      0xffd4d4d4 // texto principal
#define SIM_TEXT_DIM  0xff9d9d9d // texto secundario / labels

#define SIM_ACCENT    0xff007acc // UNICO acento azul (VS Code focusBorder) — bordes, valores, texto de estado
#define SIM_PRIMARY   0xff0e639c // azul mas oscuro, solo para el boton "primario" (Conectar/Capturar/Stop)
#define SIM_BTN       0xff3c3c3c // boton neutro (default de todos los demas botones)

#endif
