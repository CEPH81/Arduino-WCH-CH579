/*
 * variant.h - Placa de desarrollo CH579M V1.6
 *
 * Numeracion de pines de Arduino:
 *   PA0..PA15  ->  0..15
 *   PB0..PB23  -> 16..39
 * En los sketches se puede usar el numero o el nombre: digitalWrite(PB0, HIGH)
 *
 * Mapeo tomado del esquema "CH579M V1.6 (出售版)":
 *   LED de usuario (LED2) ... PB0  (se enciende con HIGH)
 *   CH340N (Serial) ......... UART1: PA9 = TX, PA8 = RX
 *   USB nativo .............. PB10 = D-, PB11 = D+
 *   Ethernet RJ45 ........... PB12..PB15 (pares TX/RX del PHY interno)
 *   SWD (conector P3) ....... PB16 = SWDIO, PB17 = SWCLK
 *   BOOT0 ................... PB22
 *   RST# (boton RST) ........ PB23
 *   Cristal 32.768 kHz ...... PA10 / PA11
 */
#ifndef VARIANT_CH579M_V16_H
#define VARIANT_CH579M_V16_H

#include <stdint.h>

/* ---------------------------------------------------------------- pines */
#define NUM_DIGITAL_PINS        40
#define NUM_ANALOG_INPUTS       14

#define PA0   0
#define PA1   1
#define PA2   2
#define PA3   3
#define PA4   4
#define PA5   5
#define PA6   6
#define PA7   7
#define PA8   8
#define PA9   9
#define PA10  10
#define PA11  11
#define PA12  12
#define PA13  13
#define PA14  14
#define PA15  15
#define PB0   16
#define PB1   17
#define PB2   18
#define PB3   19
#define PB4   20
#define PB5   21
#define PB6   22
#define PB7   23
#define PB8   24
#define PB9   25
#define PB10  26
#define PB11  27
#define PB12  28
#define PB13  29
#define PB14  30
#define PB15  31
#define PB16  32
#define PB17  33
#define PB18  34
#define PB19  35
#define PB20  36
#define PB21  37
#define PB22  38
#define PB23  39

/* ------------------------------------------------------- LEDs y botones */
#define LED_BUILTIN             PB0
#define LED2                    PB0

/* LEDs del conector RJ45. El esquema los marca en PB5/PB6, pero en la placa
 * medida con multimetro estan en PB4 (verde) y PB7 (amarillo). Ambos se
 * encienden con LOW. Nota: PB4/PB7 tambien son RX0/TX0 de Serial0. */
#define LED_ETH_GREEN           PB4
#define LED_ETH_YELLOW          PB7

#define PIN_BOOT0               PB22

/* ------------------------------------------------- entradas analogicas
 * El numero de canal (AINx) no coincide con el orden de los pines:
 *   A0=PA4  A1=PA5  A2=PA12 A3=PA13 A4=PA14 A5=PA15 A6=PA3
 *   A7=PA2  A8=PA1  A9=PA0  A10=PA6 A11=PA7 A12=PA8 A13=PA9
 * analogRead() recibe el pin: analogRead(A0) == analogRead(PA4)
 */
#define PIN_A0   PA4
#define PIN_A1   PA5
#define PIN_A2   PA12
#define PIN_A3   PA13
#define PIN_A4   PA14
#define PIN_A5   PA15
#define PIN_A6   PA3
#define PIN_A7   PA2
#define PIN_A8   PA1
#define PIN_A9   PA0
#define PIN_A10  PA6
#define PIN_A11  PA7
#define PIN_A12  PA8
#define PIN_A13  PA9

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6;
static const uint8_t A7  = PIN_A7;
static const uint8_t A8  = PIN_A8;
static const uint8_t A9  = PIN_A9;
static const uint8_t A10 = PIN_A10;
static const uint8_t A11 = PIN_A11;
static const uint8_t A12 = PIN_A12;
static const uint8_t A13 = PIN_A13;

/* ------------------------------------------------------------- Serial
 *   Serial  = UART1  TX=PA9  RX=PA8   (via CH340N al USB "serie")
 *   Serial0 = UART0  TX=PB7  RX=PB4
 *   Serial2 = UART2  TX=PA7  RX=PA6
 *   Serial3 = UART3  TX=PA5  RX=PA4
 */
#define PIN_SERIAL_TX           PA9
#define PIN_SERIAL_RX           PA8
#define PIN_SERIAL0_TX          PB7
#define PIN_SERIAL0_RX          PB4
#define PIN_SERIAL2_TX          PA7
#define PIN_SERIAL2_RX          PA6
#define PIN_SERIAL3_TX          PA5
#define PIN_SERIAL3_RX          PA4

/* ---------------------------------------------------------------- SPI
 * SPI  = SPI0: SS=PA12 SCK=PA13 MOSI=PA14 MISO=PA15
 * SPI1 = SPI1:         SCK=PA0  MOSI=PA1  MISO=PA2  (solo maestro)
 */
#define PIN_SPI_SS              PA12
#define PIN_SPI_SCK             PA13
#define PIN_SPI_MOSI            PA14
#define PIN_SPI_MISO            PA15
#define PIN_SPI1_SCK            PA0
#define PIN_SPI1_MOSI           PA1
#define PIN_SPI1_MISO           PA2

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t SCK  = PIN_SPI_SCK;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;

/* ---------------------------------------------------------------- I2C
 * El CH579 no tiene I2C por hardware: Wire es por software (bit-banging)
 * y admite cualquier par de pines con Wire.setPins(sda, scl).
 */
#define PIN_WIRE_SDA            PB21
#define PIN_WIRE_SCL            PB20

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

#ifdef __cplusplus
extern "C" {
#endif

/* Descripcion de cada pin (variant.cpp) */
typedef struct {
    uint8_t  port;        /* 0 = GPIOA, 1 = GPIOB                         */
    uint8_t  bit;         /* bit dentro del puerto (0..23)                */
    int8_t   adc_channel; /* AINx o -1                                    */
    int8_t   pwm_channel; /* PWMx (4..11) o -1                            */
} ch579_pin_t;

extern const ch579_pin_t g_pinDescription[NUM_DIGITAL_PINS];

#ifdef __cplusplus
}
#endif

#endif /* VARIANT_CH579M_V16_H */
