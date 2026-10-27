/*
 * EEPROM.cpp - EEPROM emulada en DataFlash con FlashBlockErase()/FlashWriteBuf()
 */
#include "EEPROM.h"

EEPROMClass EEPROM;

static uint32_t s_block[CH579_EEPROM_BLOCK / 4];   /* copia de un bloque */

static inline const volatile uint8_t *ee_ptr(int idx)
{
    return (const volatile uint8_t *)(CH579_EEPROM_BASE + (uint32_t)idx);
}

uint8_t EERef::operator*() const { return EEPROM.read(index); }
EERef &EERef::operator=(uint8_t v) { EEPROM.write(index, v); return *this; }

uint8_t EEPROMClass::read(int idx)
{
    if (idx < 0 || idx >= CH579_EEPROM_SIZE) return 0xFF;
    return *ee_ptr(idx);
}

void EEPROMClass::readBytes(int idx, void *data, size_t len)
{
    uint8_t *d = (uint8_t *)data;
    for (size_t i = 0; i < len; i++) d[i] = read(idx + (int)i);
}

void EEPROMClass::write(int idx, uint8_t val)
{
    writeBytes(idx, &val, 1);
}

bool EEPROMClass::writeBytes(int idx, const void *data, size_t len)
{
    if (idx < 0 || len == 0) return false;
    if ((size_t)idx + len > CH579_EEPROM_SIZE) len = CH579_EEPROM_SIZE - (size_t)idx;
    const uint8_t *src = (const uint8_t *)data;
    bool ok = true;

    while (len > 0) {
        int blk = idx / CH579_EEPROM_BLOCK;
        int off = idx % CH579_EEPROM_BLOCK;
        size_t n = CH579_EEPROM_BLOCK - (size_t)off;
        if (n > len) n = len;

        /* Si los datos no cambian, no se toca la flash */
        bool changed = false;
        for (size_t i = 0; i < n; i++) {
            if (*ee_ptr(idx + (int)i) != src[i]) { changed = true; break; }
        }

        if (changed) {
            uint32_t addr = CH579_EEPROM_BASE + (uint32_t)blk * CH579_EEPROM_BLOCK;
            uint8_t *b = (uint8_t *)s_block;
            for (int i = 0; i < CH579_EEPROM_BLOCK; i++) b[i] = *(const volatile uint8_t *)(addr + (uint32_t)i);
            for (size_t i = 0; i < n; i++) b[off + i] = src[i];

            uint8_t e = FlashBlockErase(addr);
            uint8_t w = e ? 1 : FlashWriteBuf(addr, (PUINT32)s_block, CH579_EEPROM_BLOCK);
            if (e || w) ok = false;
        }

        idx += (int)n;
        src += n;
        len -= n;
    }
    return ok;
}
