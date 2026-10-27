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
  if (!Ethernet.begin()) Ethernet.begin(IPAddress(192, 168, 1, 200));
  servidor.begin();
  Serial.print("Servidor de eco en ");
  Serial.print(Ethernet.localIP());
  Serial.println(":2000");
}

void loop() {
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
