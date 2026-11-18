/*
 * Cliente Modbus RTU minimo sobre un puerto COM de Windows — equivalente en
 * C a lo que hace `minimalmodbus` en RD90PRO_V9_CONSOLA_VIRTUAL.py. Solo
 * implementa lo que ese script usa: FC03 (leer holding), FC04 (leer input),
 * FC06 (escribir un registro), mas listar puertos COM disponibles.
 *
 * Esto habla con el ROBOT REAL por USB-RS485 — es un protocolo y un
 * dispositivo totalmente distintos del UART "HMI_REG_*" que el firmware
 * real usa hacia la placa de CONSOLA (ver main.c). No tiene nada que ver
 * con el resto del simulador (que solo imita la UI del HMI).
 */

#ifndef SIM_MODBUS_RTU_H
#define SIM_MODBUS_RTU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct sim_modbus sim_modbus_t;

// Abre el puerto (ej. "COM12") y lo configura 8N1 al baudrate pedido.
// Devuelve NULL si no se pudo abrir/configurar.
sim_modbus_t *sim_modbus_open(const char *port, int baud);
void sim_modbus_close(sim_modbus_t *m);

// FC06 — escribe un registro. true si la respuesta fue valida.
bool sim_modbus_write_register(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t value);

// FC04 / FC03 — leen 'count' registros a partir de 'reg' en out[].
bool sim_modbus_read_input_registers(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t count, uint16_t *out);
bool sim_modbus_read_holding_registers(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t count, uint16_t *out);

// Lista los puertos COM presentes en el sistema (HKLM\HARDWARE\DEVICEMAP\SERIALCOMM).
// Llena out[i] (buffers de 'name_len' bytes) hasta 'max_ports'. Devuelve cuantos encontro.
int sim_modbus_list_ports(char out[][16], int max_ports);

#endif
