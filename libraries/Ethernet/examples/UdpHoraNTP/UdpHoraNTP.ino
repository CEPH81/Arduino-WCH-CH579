/*
 * UdpHoraNTP - Consulta la hora a un servidor NTP (UDP 123) cada 10 s y la
 * muestra por Serial (UTC; ajuste ZONA_HORARIA).
 */
#include <Ethernet.h>

const char *SERVIDOR_NTP = "pool.ntp.org";
const int   ZONA_HORARIA = -4;          // horas respecto a UTC (Venezuela: -4)

EthernetUDP udp;
uint8_t paquete[48];

bool pedirHora(uint32_t &unixUtc) {
  memset(paquete, 0, sizeof(paquete));
  paquete[0] = 0b11100011;              // LI=3, version 4, modo cliente
  if (!udp.beginPacket(SERVIDOR_NTP, 123)) return false;
  udp.write(paquete, sizeof(paquete));
  udp.endPacket();

  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    if (udp.parsePacket() >= 48) {
      udp.read(paquete, 48);
      uint32_t ntp = ((uint32_t)paquete[40] << 24) | ((uint32_t)paquete[41] << 16) |
                     ((uint32_t)paquete[42] << 8)  |  (uint32_t)paquete[43];
      unixUtc = ntp - 2208988800UL;     // NTP cuenta desde 1900, Unix desde 1970
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  if (!Ethernet.begin()) {
    Serial.println("DHCP fallo: NTP necesita salida a Internet");
    while (true) delay(1000);
  }
  udp.begin(8888);
}

void loop() {
  uint32_t t;
  if (pedirHora(t)) {
    t += ZONA_HORARIA * 3600L;
    char hora[16];
    snprintf(hora, sizeof(hora), "%02lu:%02lu:%02lu",
             (unsigned long)((t % 86400UL) / 3600), (unsigned long)((t % 3600) / 60), (unsigned long)(t % 60));
    Serial.print("Hora: ");
    Serial.print(hora);
    Serial.print("   (Unix ");
    Serial.print(t);
    Serial.println(")");
  } else {
    Serial.println("Sin respuesta NTP");
  }
  delay(10000);
}
