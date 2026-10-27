/*
 * EEPROM.h - EEPROM emulada en la DataFlash del CH579 (2 KB en 0x3E800)
 *
 * Compatible con la API de AVR: EEPROM.read/write/update/get/put y EEPROM[i].
 * La DataFlash se borra en bloques de 512 bytes: cada escritura que cambia
 * un valor reescribe su bloque (~ms). Solo se borra si el dato es distinto,
 * pero evite escribir en bucles rapidos (vida util ~10.000 ciclos/bloque).
 */
#ifndef CH579_EEPROM_H
#define CH579_EEPROM_H

#include <Arduino.h>

#define CH579_EEPROM_BASE   0x0003E800UL
#define CH579_EEPROM_SIZE   2048
#define CH579_EEPROM_BLOCK  512

class EEPROMClass;

struct EERef {
    EERef(int i) : index(i) { }
    uint8_t operator*() const;
    operator uint8_t() const { return **this; }
    EERef &operator=(const EERef &r) { return *this = *r; }
    EERef &operator=(uint8_t v);
    EERef &operator+=(uint8_t v) { return *this = **this + v; }
    EERef &operator-=(uint8_t v) { return *this = **this - v; }
    EERef &operator|=(uint8_t v) { return *this = **this | v; }
    EERef &operator&=(uint8_t v) { return *this = **this & v; }
    EERef &operator++() { return *this += 1; }
    EERef &operator--() { return *this -= 1; }
    uint8_t operator++(int) { uint8_t r = **this; ++(*this); return r; }
    uint8_t operator--(int) { uint8_t r = **this; --(*this); return r; }
    EERef &update(uint8_t v) { return *this = v; }
    int index;
};

class EEPROMClass
{
public:
    uint8_t read(int idx);
    void    write(int idx, uint8_t val);
    void    update(int idx, uint8_t val) { write(idx, val); }
    EERef   operator[](int idx) { return EERef(idx); }
    uint16_t length() { return CH579_EEPROM_SIZE; }

    /* Escribe un bloque de bytes con un borrado por bloque de 512 B.
     * Devuelve true si todo se grabo correctamente. */
    bool writeBytes(int idx, const void *data, size_t len);
    void readBytes(int idx, void *data, size_t len);

    template <typename T> T &get(int idx, T &t) {
        readBytes(idx, &t, sizeof(T));
        return t;
    }
    template <typename T> const T &put(int idx, const T &t) {
        writeBytes(idx, &t, sizeof(T));
        return t;
    }
};

extern EEPROMClass EEPROM;

#endif
