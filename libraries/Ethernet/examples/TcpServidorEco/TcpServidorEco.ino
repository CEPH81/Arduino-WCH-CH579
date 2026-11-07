/*
 * TcpServidorEco - Servidor TCP en el puerto 2000 que devuelve todo lo que
 * recibe (admite varios clientes a la vez).
 *
 * Desde el PC:   ncat <IP de la placa> 2000
 * o Hercules en modo "TCP Client".
 */
#include <Ethernet.h>

EthernetServer servidor(2000);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M Ethernet: TcpServidorEco ===");
  Ethernet.setDebug(Serial);        // muestra cada paso: enlace, DHCP...
  Ethernet.beginAuto();            // DHCP, o 169.254.x.y si va directo al PC
  servidor.begin();
  Serial.print("Servidor de eco en ");
  Serial.print(Ethernet.localIP());
  Serial.println(":2000");
}

/* Repite la direccion cada 10 s (por si abrio el Monitor Serie tarde) */
void recordarIP() {
  static unsigned long t = 0;
  if (millis() - t < 10000) return;
  t = millis();
  Serial.print("Servidor de eco en ");
  Serial.print(Ethernet.localIP());
  Serial.println(":2000");
}

void loop() {
  recordarIP();
  /* Avisa de cada conexion nueva */
  EthernetClient nuevo = servidor.accept();
  if (nuevo) {
    Serial.print("Conexion de ");
    Serial.print(nuevo.remoteIP());
    Serial.print(":");
    Serial.println(nuevo.remotePort());
    nuevo.println("Bienvenido al eco de la CH579M");
  }

  /* Eco de los clientes con datos */
  EthernetClient c = servidor.available();
  if (c) {
    uint8_t buf[128];
    int n = c.read(buf, sizeof(buf));
    if (n > 0) c.write(buf, n);
  }
}
