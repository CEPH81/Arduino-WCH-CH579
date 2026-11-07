/*
 * TcpCliente - Cliente TCP: se conecta a un servidor en el PC, envia un
 * saludo cada 5 s y muestra por Serial lo que el servidor responda.
 *
 * En el PC abra un servidor TCP en el puerto 5000, por ejemplo con
 * Hercules (pestana "TCP Server") o:   ncat -l -k 5000
 * y ponga abajo la IP de su PC.
 */
#include <Ethernet.h>

IPAddress servidor(192, 168, 1, 100);   // <-- IP de su PC
const uint16_t PUERTO = 5000;

EthernetClient cliente;
unsigned long ultimoEnvio = 0;

void conectar() {
  Serial.print("Conectando a ");
  Serial.print(servidor);
  Serial.print(":");
  Serial.print(PUERTO);
  Serial.print(" ... ");
  if (cliente.connect(servidor, PUERTO)) {
    Serial.println("OK");
    cliente.println("Hola desde la CH579M");
  } else {
    Serial.println("fallo");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M Ethernet: TcpCliente ===");
  Ethernet.setDebug(Serial);        // muestra cada paso: enlace, DHCP...
  Ethernet.beginAuto();            // DHCP, o 169.254.x.y si va directo al PC
  Serial.print("Mi IP: ");
  Serial.println(Ethernet.localIP());
  conectar();
}

void loop() {
  while (cliente.available()) {                 // lo que llega del servidor
    Serial.write(cliente.read());
  }

  if (!cliente.connected()) {
    cliente.stop();
    delay(2000);
    conectar();                                 // reintenta si se corto
    return;
  }

  if (millis() - ultimoEnvio > 5000) {
    ultimoEnvio = millis();
    cliente.print("millis = ");
    cliente.println(millis());
  }
}
