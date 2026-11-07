/*
 * UdpEco - Recibe paquetes UDP en el puerto 2000 y responde al remitente
 * con el mismo texto en mayusculas.
 *
 * Desde el PC:   ncat -u <IP de la placa> 2000
 * o Hercules en la pestana "UDP".
 */
#include <Ethernet.h>

EthernetUDP udp;
char paquete[256];

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M Ethernet: UdpEco ===");
  Ethernet.setDebug(Serial);        // muestra cada paso: enlace, DHCP...
  Ethernet.beginAuto();            // DHCP, o 169.254.x.y si va directo al PC
  udp.begin(2000);
  Serial.print("UDP en ");
  Serial.print(Ethernet.localIP());
  Serial.println(":2000");
}

/* Repite la direccion cada 10 s (por si abrio el Monitor Serie tarde) */
void recordarIP() {
  static unsigned long t = 0;
  if (millis() - t < 10000) return;
  t = millis();
  Serial.print("UDP en ");
  Serial.print(Ethernet.localIP());
  Serial.println(":2000");
}

void loop() {
  recordarIP();
  int n = udp.parsePacket();
  if (n > 0) {
    int len = udp.read(paquete, sizeof(paquete) - 1);
    if (len < 0) len = 0;
    paquete[len] = 0;
    Serial.print("De ");
    Serial.print(udp.remoteIP());
    Serial.print(":");
    Serial.print(udp.remotePort());
    Serial.print(" -> ");
    Serial.println(paquete);

    for (int i = 0; i < len; i++) paquete[i] = toupper(paquete[i]);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t *)paquete, len);
    udp.endPacket();
  }
}
