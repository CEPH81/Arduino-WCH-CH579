/*
 * EthernetInfo - Diagnostico de la red: enlace, DHCP y configuracion.
 *
 * Conecte el RJ45 de la placa a su router/switch (o directo al puerto LAN del
 * PC, ver ConexionDirectaPC) y abra el Monitor Serie
 * (puerto del CH340, 115200). Muestra cada paso; si algo falla dice donde.
 * Con IP asignada, haga "ping" a esa IP desde el PC.
 */
#include <Ethernet.h>

void printMAC() {
  uint8_t mac[6];
  Ethernet.MACAddress(mac);
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print('0');
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();
}

void printPHY() {
  uint16_t bmsr = Ethernet.phyRegister(1);
  Serial.print("PHY: estado pila=");
  Serial.print(Ethernet.phyStatus());
  Serial.print("  BMCR=0x");
  Serial.print(Ethernet.phyRegister(0), HEX);
  Serial.print("  BMSR=0x");
  Serial.print(bmsr, HEX);
  Serial.println((bmsr & 0x0004) ? "  (enlace SI)" : "  (enlace NO)");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M Ethernet: EthernetInfo ===");
  Ethernet.setDebug(Serial);        // muestra cada paso: enlace, DHCP...

  bool dhcp = Ethernet.begin(nullptr, 20000);   // DHCP con la MAC unica del chip
  Serial.print("MAC:       "); printMAC();
  printPHY();
  if (!dhcp) {
    /* sin router (por ejemplo, cable directo al PC): IP 169.254.x.y, la misma
       red que se pone el PC solo en su puerto LAN */
    Ethernet.beginLinkLocal();
  }
  Serial.print("IP:        "); Serial.println(Ethernet.localIP());
  Serial.print("Mascara:   "); Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway:   "); Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS:       "); Serial.println(Ethernet.dnsServerIP());
  Serial.println("Haga ping a esa IP desde el PC.");
}

void loop() {
  static EthernetLinkStatus antes = Unknown;
  EthernetLinkStatus ahora = Ethernet.linkStatus();
  if (ahora != antes) {
    Serial.println(ahora == LinkON ? "Cable conectado" : "Cable desconectado");
    printPHY();
    antes = ahora;
  }
  delay(100);
}
