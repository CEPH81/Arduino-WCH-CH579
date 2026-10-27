/*
 * EthernetInfo - Obtiene IP por DHCP y muestra la configuracion de red.
 *
 * Conecte el RJ45 de la placa a su router/switch. Resultado por Serial
 * (115200). Despues puede hacer "ping" a la IP mostrada desde el PC.
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

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\nCH579M Ethernet - DHCP");

  if (!Ethernet.begin()) {            // DHCP con la MAC unica del chip
    Serial.print("DHCP fallo. Enlace: ");
    Serial.println(Ethernet.linkStatus() == LinkON ? "SI" : "NO (revise el cable)");
    while (true) delay(1000);
  }
  Serial.print("MAC:       "); printMAC();
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
    antes = ahora;
  }
  delay(100);
}
