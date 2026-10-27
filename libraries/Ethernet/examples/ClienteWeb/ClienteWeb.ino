/*
 * ClienteWeb - Peticion HTTP GET a un servidor web (resuelto por DNS) y
 * muestra la respuesta por Serial.
 *
 * Usa HTTP sin cifrar (puerto 80): el CH579 no tiene capacidad para HTTPS.
 * Servidores de prueba: example.com, httpbin.org/ip, o uno en su PC.
 */
#include <Ethernet.h>

const char *HOST = "example.com";
const char *RUTA = "/";

EthernetClient cliente;

void setup() {
  Serial.begin(115200);
  if (!Ethernet.begin()) {
    Serial.println("DHCP fallo: este ejemplo necesita salida a Internet");
    while (true) delay(1000);
  }
  Serial.print("Mi IP: ");
  Serial.println(Ethernet.localIP());

  IPAddress ip;
  if (Ethernet.hostByName(HOST, ip)) {
    Serial.print(HOST);
    Serial.print(" -> ");
    Serial.println(ip);
  }

  if (cliente.connect(HOST, 80)) {
    cliente.print("GET ");
    cliente.print(RUTA);
    cliente.println(" HTTP/1.1");
    cliente.print("Host: ");
    cliente.println(HOST);
    cliente.println("User-Agent: CH579M");
    cliente.println("Connection: close");
    cliente.println();
  } else {
    Serial.println("No se pudo conectar");
  }
}

void loop() {
  while (cliente.available()) {
    Serial.write(cliente.read());
  }
  if (!cliente.connected()) {
    Serial.println("\r\n--- fin de la respuesta ---");
    cliente.stop();
    while (true) delay(1000);
  }
}
