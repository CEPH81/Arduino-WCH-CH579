# arduino-wch-ch579

Paquete de placas para **Arduino IDE 2.x** con soporte del microcontrolador **WCH CH579**
(ARM Cortex-M0, 250 KB de flash, 32 KB de RAM). Primera placa: **CH579M, placa de desarrollo V1.6**.

- Carga del programa con **SEGGER J-Link** por SWD, sin seleccionar puerto.
- **Depuración** desde el IDE 2.x (puntos de interrupción, variables y registros de periféricos con el SVD).
- API de Arduino estándar (basada en ArduinoCore-API) + acceso directo al SDK de WCH desde el sketch.

> Versión 0.1.0, **en pruebas**: compila con GCC 7 y GCC 13 y se probó en un emulador
> de Cortex-M0, pero **aún no se ha probado en la placa real**.

---

## Instalación (repositorio privado, instalación manual)

1. **Arduino IDE 2.x** instalado.
2. **Compilador ARM**: en *Herramientas → Placa → Gestor de placas*, instale
   **"Arduino SAMD Boards (32-bits ARM Cortex-M0+)"**. Solo se usa para obtener
   `arm-none-eabi-gcc`; no hace falta usar esas placas.
3. **Este paquete**: copie o clone el repositorio exactamente en esta ruta
   (los nombres `wch` y `ch57x` son obligatorios):

   ```
   Windows:  Documentos\Arduino\hardware\wch\ch57x\
   Linux:    ~/Arduino/hardware/wch/ch57x/
   ```

   Dentro de `ch57x` deben quedar `boards.txt`, `platform.txt`, `cores`, `variants`...

   ```
   cd Documentos\Arduino\hardware\wch
   git clone https://github.com/<usuario>/arduino-wch-ch579.git ch57x
   ```
4. **J-Link**: instale *J-Link Software and Documentation Pack* de SEGGER.
   El script de carga registra solo la definición del CH579M (`JLinkDevices.xml`
   + `CH57x.FLM`) en `%APPDATA%\SEGGER\JLinkDevices\WCH\CH579M\`.
5. Reinicie el IDE y elija **Herramientas → Placa → WCH CH57x → WCH CH579M (placa de desarrollo V1.6)**.

### Conexión del J-Link (conector P3 de la placa)

| J-Link | CH579M |
|---|---|
| SWDIO | PB16 |
| SWCLK | PB17 |
| GND | GND |
| VTref | 3V3 |

Pulse **Subir**: no hace falta elegir puerto. Para el **Monitor Serie**, elija el
puerto COM del CH340 (USB "serie" de la placa) a 115200 baudios.

Si `JLink.exe` está en una ruta no estándar, defina la variable de entorno
`JLINK_PATH` con la carpeta que lo contiene.

---

## Menús de la placa

| Menú | Opciones |
|---|---|
| Reloj del sistema | **Cristal 32 MHz (HSE)** · PLL 40 MHz · RC interno 32 MHz |
| Optimización | **-Os** · -O2 · -Og (depuración) |
| Velocidad SWD (J-Link) | **4000** · 1000 · 200 kHz |

---

## Pines (placa V1.6)

Numeración de Arduino: **PA0..PA15 = 0..15**, **PB0..PB23 = 16..39**.
En el sketch se puede escribir el nombre directamente: `digitalWrite(PB0, HIGH)`.

| Función | Pines |
|---|---|
| LED de usuario (`LED_BUILTIN`) | PB0 (LED2, enciende con HIGH) |
| `Serial` (UART1 → CH340N) | TX = PA9, RX = PA8 |
| `Serial0` (UART0) | TX = PB7, RX = PB4 |
| `Serial2` (UART2) | TX = PA7, RX = PA6 |
| `Serial3` (UART3) | TX = PA5, RX = PA4 |
| `SerialUSB` (USB nativo, 2.º micro-USB) | D- = PB10, D+ = PB11 |
| `SPI` (SPI0) | SS = PA12, SCK = PA13, MOSI = PA14, MISO = PA15 |
| `SPI1` | SCK = PA0, MOSI = PA1, MISO = PA2 |
| `Wire` (I²C por software) | SDA = PB21, SCL = PB20 (cambiables con `Wire.setPins`) |
| Entradas analógicas | A0=PA4 A1=PA5 A2=PA12 A3=PA13 A4=PA14 A5=PA15 A6=PA3 A7=PA2 A8=PA1 A9=PA0 A10=PA6 A11=PA7 A12=PA8 A13=PA9 |
| PWM por hardware (`analogWrite`) | PA6 PA7 PB0 PB1 PB2 PB3 PB14 (PB23) |
| Interrupciones (`attachInterrupt`) | PA0..PA15, PB0..PB15 |
| LEDs del RJ45 | PB4 (verde), PB7 (amarillo), encienden con LOW* |
| Reservados en la placa | PA10/PA11 (cristal 32 kHz), PB10/PB11 (USB), PB12..PB15 (Ethernet), **PB16/PB17 (SWD)**, PB22 (BOOT0), PB23 (RST#) |

\* El esquema indica PB5/PB6, pero en la placa medida están en PB4/PB7. Confírmelo en la suya.

> No use **PB16/PB17** en el sketch: son el SWD y perdería la carga y la depuración
> con J-Link (habría que recuperar la placa con el bootloader USB de WCH).

---

## Qué incluye

| Función | Estado |
|---|---|
| `pinMode` (INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT, OUTPUT_OPENDRAIN\*, `OUTPUT_20MA`), `digitalWrite`, `digitalRead` | ✅ |
| `millis`, `micros`, `delay`, `delayMicroseconds` (SysTick) | ✅ |
| `Serial`, `Serial0`, `Serial2`, `Serial3` (RX por interrupción, 256 B) + `printf()` por Serial | ✅ |
| `analogRead` (12 bits, 10 por defecto), `analogReadMillivolts`, `analogReference` (ganancia del PGA) | ✅ |
| `analogWrite` (PWM 8 bits), `analogWriteFrequency`, `analogWriteResolution` | ✅ |
| `attachInterrupt` (LOW, HIGH, RISING, FALLING, CHANGE\*\*) | ✅ |
| `tone` / `noTone` (TMR3), `pulseIn`, `shiftOut` / `shiftIn`, `random` | ✅ |
| Librerías **SPI**, **Wire**, **EEPROM** (2 KB de DataFlash) | ✅ |
| **LCD_HD44780** (LCD 16x2 por I²C PCF8574 o 74HC164), **OLED_SSD1306**, **HCSR04_CH579**, **NTC_Thermistor**, **CH579_Log** | ✅ portadas de los proyectos Keil |
| Funciones del SDK de WCH (`GPIOA_ModeCfg`, `TMR0_TimerInit`, `GetSysClock`...) | ✅ disponibles en el sketch |
| `SerialUSB`: puerto COM virtual (CDC) por el USB nativo (PB10/PB11), sin drivers | ✅ |
| **Ethernet** (10 Mbit/s integrado): `Ethernet`, `EthernetClient`, `EthernetServer`, `EthernetUDP`, DHCP y DNS | ✅ librería **Ethernet** propia del CH579 |
| BLE | ⏳ siguiente fase |

\* Emulado: el CH579 no tiene salida de drenador abierto.
\*\* Emulado invirtiendo el flanco en cada interrupción.

### Ethernet (librería `Ethernet` del CH579)

Usa el PHY y MAC de 10 Mbit/s del CH579 con la pila TCP/IP de WCH (`CH57xNET.LIB`,
ya incluida como `libCH57xNET.a`). La API es la de la librería `Ethernet` de Arduino,
así que funcionan las librerías que trabajan con un `Client` (por ejemplo **PubSubClient**
para MQTT).

```cpp
#include <Ethernet.h>
Ethernet.begin();                                  // DHCP con la MAC de fábrica del chip
Ethernet.begin(IPAddress(192, 168, 1, 200));       // o IP fija
```

| Ejemplo (Archivo → Ejemplos → Ethernet, en la sección de la placa WCH CH579M) | Qué hace |
|---|---|
| EthernetInfo | Estado del enlace, IP, puerta de enlace, DNS y MAC |
| TcpCliente | Cliente TCP que se conecta a un servidor del PC (192.168.1.100:5000) |
| TcpServidorEco | Servidor TCP de eco en el puerto 2000 (varios clientes) |
| ServidorWeb | Página web en el puerto 80: lectura de A0 y botones para el LED |
| ClienteWeb | Petición HTTP a `example.com` (DNS + TCP) |
| UdpEco | Recibe UDP en el puerto 2000 y responde en mayúsculas |
| UdpHoraNTP | Hora de `pool.ntp.org` por NTP |
| MQTT_PubSub | Publica A0 y el tiempo encendido, y controla el LED por MQTT (requiere **PubSubClient**) |

Se llama `Ethernet` a propósito: con la placa CH579M elegida, el IDE la usa en lugar de la
librería `Ethernet` genérica de Arduino (W5100/W5500), aunque esa también esté instalada.

Para ver qué pasa al arrancar, los ejemplos llaman a `Ethernet.setDebug(Serial)`: se imprime
cada paso (`[ETH] Esperando enlace...`, `[ETH] Pidiendo IP por DHCP...`). Si no aparece nada,
abra el Monitor Serie y pulse **RESET** en la placa. **EthernetInfo** sirve de diagnóstico:
muestra el estado del PHY (registros BMCR/BMSR) aunque no haya IP.

Límites: 6 sockets en total (por ejemplo 1 servidor + 4 conexiones TCP + UDP);
la librería ocupa ~24 KB de RAM, quedan ~7–8 KB para el sketch. Los LEDs del RJ45
los controla el sketch (ver ejemplo `08_LedsRJ45`); no los usa la pila.

### Detalles del hardware a tener en cuenta

- **ADC**: con la ganancia por defecto (`AR_DEFAULT`) mide de **0 a ~2,1 V**.
  Para tensiones mayores use un divisor o `analogReference(AR_GAIN_1_2)` (1,9–3 V)
  o `AR_GAIN_1_4` (2,9 V–VIO33). La calibración de offset del ADC usa **PA5 (A1)**:
  déjelo sin tensión externa.
- **SPI**: el hardware solo tiene los modos **0 y 3** (los modos 1 y 2 se convierten en 0 y 3).
- **I²C**: el CH579 no tiene I²C por hardware; `Wire` es por software (solo maestro).
  Use resistencias de pull-up externas de 4,7 kΩ.
- **PWM**: los 8 canales comparten la frecuencia (por defecto 1 kHz, de ~490 Hz a 125 kHz a 32 MHz).

---

## Depuración en el IDE 2.x

1. Menú **Optimización → Depuración (-Og)**, o marque *Sketch → Optimizar para depuración*.
2. Pulse el botón de depurar. El IDE usa `JLinkGDBServerCL` y el archivo `svd/CH579SFR.svd`
   para mostrar los registros de los periféricos.

Si su J-Link está instalado en una carpeta con versión (por ejemplo `JLink_V812`),
cree `platform.local.txt` junto a `platform.txt` con:

```
debug.server.jlink.path=C:/Program Files/SEGGER/JLink_V812/JLinkGDBServerCL.exe
```

---

## Estructura

```
boards.txt / platform.txt     definición de la placa y del proceso de compilación
cores/arduino/                core Arduino + ArduinoCore-API (api/) + SDK de WCH (sdk/)
variants/ch579m_v16/          mapa de pines y linker script (ch579.ld)
libraries/                    SPI, Wire, EEPROM, Ethernet (CH579), LCD_HD44780, OLED_SSD1306,
                              HCSR04_CH579, NTC_Thermistor, CH579_Log y los ejemplos de la placa (CH579M)
tools/jlink/                  scripts de carga + JLinkDevices.xml + CH57x.FLM
svd/CH579SFR.svd              registros para el depurador
```

## Licencias

- `cores/arduino/api/`: ArduinoCore-API, LGPL 2.1.
- `cores/arduino/sdk/`, `libraries/Ethernet/src/cortex-m0/libCH57xNET.a`, `CH57xNET.h`, `tools/jlink/CH579M/CH57x.FLM`, `svd/`: material de WCH. Su licencia
  no está especificada; el repositorio es privado mientras se aclara la redistribución.
- El resto (core, variante, librerías y scripts): escrito para este proyecto. Las librerías
  de pantallas y sensores están portadas de la HAL `Hal_CH57xx_*` de Carlos Pacheco (CEPH81).
