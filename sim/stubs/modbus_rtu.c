/*
 * Ver modbus_rtu.h. Implementacion sobre la API nativa de puertos serie de
 * Windows (CreateFile/SetCommState/ReadFile/WriteFile) — no depende de
 * ninguna libreria externa, solo <windows.h>.
 */

#include "modbus_rtu.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sim_modbus {
    HANDLE h;
};

// CRC16 Modbus estandar (polinomio 0xA001, igual que usa cualquier libreria
// Modbus RTU, incluida minimalmodbus).
static uint16_t modbus_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)data[pos];
        for (int i = 8; i != 0; i--) {
            if (crc & 1) { crc >>= 1; crc ^= 0xA001; }
            else crc >>= 1;
        }
    }
    return crc;
}

sim_modbus_t *sim_modbus_open(const char *port, int baud)
{
    char path[32];
    snprintf(path, sizeof(path), "\\\\.\\%s", port);

    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                            OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;

    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return NULL; }
    dcb.BaudRate = (DWORD)baud;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_DISABLE;
    if (!SetCommState(h, &dcb)) { CloseHandle(h); return NULL; }

    COMMTIMEOUTS to;
    memset(&to, 0, sizeof(to));
    to.ReadIntervalTimeout        = 50;
    to.ReadTotalTimeoutConstant   = 300;
    to.ReadTotalTimeoutMultiplier = 10;
    to.WriteTotalTimeoutConstant  = 300;
    to.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &to);

    sim_modbus_t *m = (sim_modbus_t *)calloc(1, sizeof(*m));
    m->h = h;
    return m;
}

void sim_modbus_close(sim_modbus_t *m)
{
    if (!m) return;
    if (m->h && m->h != INVALID_HANDLE_VALUE) CloseHandle(m->h);
    free(m);
}

static bool modbus_transact(sim_modbus_t *m, const uint8_t *req, int req_len,
                             uint8_t *resp, int resp_len)
{
    if (!m || !m->h) return false;
    PurgeComm(m->h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    DWORD written = 0;
    if (!WriteFile(m->h, req, (DWORD)req_len, &written, NULL) || (int)written != req_len) {
        return false;
    }

    DWORD total = 0;
    while ((int)total < resp_len) {
        DWORD n = 0;
        if (!ReadFile(m->h, resp + total, (DWORD)(resp_len - total), &n, NULL)) return false;
        if (n == 0) break; // timeout: no llego mas
        total += n;
    }
    return (int)total == resp_len;
}

bool sim_modbus_write_register(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t value)
{
    uint8_t req[8];
    req[0] = slave_addr;
    req[1] = 0x06;
    req[2] = (uint8_t)(reg >> 8);   req[3] = (uint8_t)(reg & 0xFF);
    req[4] = (uint8_t)(value >> 8); req[5] = (uint8_t)(value & 0xFF);
    uint16_t crc = modbus_crc16(req, 6);
    req[6] = (uint8_t)(crc & 0xFF); req[7] = (uint8_t)(crc >> 8);

    uint8_t resp[8];
    if (!modbus_transact(m, req, 8, resp, 8)) return false;
    return resp[0] == slave_addr && resp[1] == 0x06;
}

static bool modbus_read_regs(sim_modbus_t *m, uint8_t slave_addr, uint8_t fc,
                              uint16_t reg, uint16_t count, uint16_t *out)
{
    if (count == 0 || count > 25) return false; // 25*2+5=55, entra en el buffer de 64
    uint8_t req[8];
    req[0] = slave_addr;
    req[1] = fc;
    req[2] = (uint8_t)(reg >> 8);   req[3] = (uint8_t)(reg & 0xFF);
    req[4] = (uint8_t)(count >> 8); req[5] = (uint8_t)(count & 0xFF);
    uint16_t crc = modbus_crc16(req, 6);
    req[6] = (uint8_t)(crc & 0xFF); req[7] = (uint8_t)(crc >> 8);

    int resp_len = 5 + count * 2;
    uint8_t resp[64];
    if (!modbus_transact(m, req, 8, resp, resp_len)) return false;
    if (resp[0] != slave_addr || resp[1] != fc) return false;

    for (uint16_t i = 0; i < count; i++) {
        out[i] = ((uint16_t)resp[3 + i * 2] << 8) | resp[4 + i * 2];
    }
    return true;
}

bool sim_modbus_read_input_registers(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t count, uint16_t *out)
{
    return modbus_read_regs(m, slave_addr, 0x04, reg, count, out);
}

bool sim_modbus_read_holding_registers(sim_modbus_t *m, uint8_t slave_addr, uint16_t reg, uint16_t count, uint16_t *out)
{
    return modbus_read_regs(m, slave_addr, 0x03, reg, count, out);
}

int sim_modbus_list_ports(char out[][16], int max_ports)
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return 0;
    }
    int count = 0;
    DWORD idx = 0;
    for (;;) {
        char valueName[256];
        char data[64];
        DWORD valueNameSize = sizeof(valueName);
        DWORD dataSize = sizeof(data);
        DWORD type = 0;
        LONG r = RegEnumValueA(hKey, idx++, valueName, &valueNameSize, NULL, &type, (BYTE *)data, &dataSize);
        if (r == ERROR_NO_MORE_ITEMS) break;
        if (r != ERROR_SUCCESS) continue;
        if (type == REG_SZ && count < max_ports) {
            strncpy(out[count], data, 15);
            out[count][15] = '\0';
            count++;
        }
    }
    RegCloseKey(hKey);
    return count;
}
