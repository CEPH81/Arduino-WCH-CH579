/*
 * ConexionDirectaPC - la CH579M conectada con un cable de red DIRECTO al
 * puerto LAN del PC, sin router (el PC puede seguir en Internet por WiFi).
 *
 * Funciona de tres formas; la placa se adapta sola (Ethernet.beginAuto()):
 *
 *  A) Sin configurar nada (lo mas facil): sin router no hay DHCP, asi que el
 *     PC se pone solo una IP 169.254.x.x en ese puerto, y la placa tambien
 *     (sacada de su MAC). Espere ~30 s tras conectar el cable y haga ping a
 *     la IP que muestra el monitor.
 *
 *  B) Compartir el WiFi del PC por el cable (la placa tiene Internet: NTP,
 *     MQTT, ClienteWeb...). En Windows: Panel de control > Redes > Cambiar
 *     opciones del adaptador > clic derecho en el adaptador WiFi > Propiedades
 *     > Uso compartido > "Permitir que otros usuarios... se conecten" y elija
 *     el adaptador Ethernet. El PC queda como 192.168.137.1 y le da IP por
 *     DHCP a la placa (192.168.137.x).
 *
 *  C) IP fija en los dos: en el adaptador Ethernet del PC ponga, por ejemplo,
 *     192.168.10.1 / 255.255.255.0 y en el sketch Ethernet.begin(IPAddress(192,168,10,2)).
 *     Use una red distinta de la del WiFi (si el WiFi es 192.168.1.x, no use 192.168.1.x).
 *
 * El cable puede ser normal (recto): los puertos del PC lo cruzan solos.
 * El CH579 es Ethernet de 10 Mbps: el PC lo negocia automaticamente.
 *
 * Abra el monitor serie (115200) y luego http://<IP de la placa> en el navegador.
 */
#include <Ethernet.h>

EthernetServer servidor(80);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M: ConexionDirectaPC ===");
  Ethernet.setDebug(Serial);
  int modo = Ethernet.beginAuto(nullptr, 15000);
  if (modo == 0) {
    Serial.println("Sin enlace: revise el cable (los LED del RJ45 deben encender).");
  } else if (modo == 1) {
    Serial.println("IP por DHCP (router o 'Uso compartido' de Windows).");
  } else {
    Serial.println("IP de enlace local: el PC debe estar en 169.254.x.x en ese puerto.");
  }
  Serial.print("IP de la placa: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Mascara:        ");
  Serial.println(Ethernet.subnetMask());
  Serial.println("Haga ping a esa IP o abrala en el navegador.");
  servidor.begin();
}

void loop() {
  EthernetClient cliente = servidor.available();
  if (cliente) {
    /* leer la peticion hasta la linea vacia */
    bool lineaVacia = true;
    while (cliente.connected()) {
      if (!cliente.available()) continue;
      char c = cliente.read();
      if (c == '\n' && lineaVacia) break;
      if (c == '\n') lineaVacia = true;
      else if (c != '\r') lineaVacia = false;
    }
    cliente.println("HTTP/1.1 200 OK");
    cliente.println("Content-Type: text/html");
    cliente.println("Connection: close");
    cliente.println();
    cliente.print("<html><body><h2>CH579M conectada directo al PC</h2><p>IP: ");
    cliente.print(Ethernet.localIP());
    cliente.print("</p><p>Encendida hace ");
    cliente.print(millis() / 1000);
    cliente.println(" s</p></body></html>");
    delay(2);
    cliente.stop();
  }
  Ethernet.maintain();
}
