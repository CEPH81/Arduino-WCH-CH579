/*
 * ServidorWeb - Pagina web en la placa: muestra A0, millis() y permite
 * encender/apagar LED2 desde el navegador.
 *
 * Abra en el navegador:  http://<IP de la placa>/
 */
#include <Ethernet.h>

EthernetServer servidor(80);

void enviarPagina(EthernetClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/html; charset=utf-8");
  c.println("Connection: close");
  c.println();
  c.println("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>");
  c.println("<meta http-equiv='refresh' content='5'><title>CH579M</title></head><body>");
  c.println("<h2>CH579M - servidor web</h2>");
  c.print("<p>A0 (PA4): <b>");
  c.print(analogReadMillivolts(A0));
  c.println(" mV</b></p>");
  c.print("<p>Tiempo encendida: ");
  c.print(millis() / 1000);
  c.println(" s</p>");
  c.print("<p>LED2: <b>");
  c.print(digitalRead(LED_BUILTIN) ? "ENCENDIDO" : "apagado");
  c.println("</b></p>");
  c.println("<p><a href='/on'>Encender</a> | <a href='/off'>Apagar</a></p>");
  c.println("</body></html>");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n=== CH579M Ethernet: ServidorWeb ===");
  Ethernet.setDebug(Serial);        // muestra cada paso: enlace, DHCP...
  pinMode(LED_BUILTIN, OUTPUT);
  if (!Ethernet.begin(nullptr, 20000)) Ethernet.begin(IPAddress(192, 168, 1, 200));
  servidor.begin();
  Serial.print("Abra http://");
  Serial.println(Ethernet.localIP());
}

/* Repite la direccion cada 10 s (por si abrio el Monitor Serie tarde) */
void recordarIP() {
  static unsigned long t = 0;
  if (millis() - t < 10000) return;
  t = millis();
  Serial.print("Servidor web en http://");
  Serial.print(Ethernet.localIP());
  Serial.println("");
}

void loop() {
  recordarIP();
  EthernetClient c = servidor.available();
  if (!c) return;

  /* Lee la primera linea de la peticion: "GET /ruta HTTP/1.1" */
  String peticion = c.readStringUntil('\n');
  Serial.println(peticion);
  if (peticion.indexOf("GET /on") >= 0)  digitalWrite(LED_BUILTIN, HIGH);
  if (peticion.indexOf("GET /off") >= 0) digitalWrite(LED_BUILTIN, LOW);

  /* Descarta el resto de las cabeceras hasta la linea vacia */
  unsigned long t0 = millis();
  while (c.connected() && millis() - t0 < 1000) {
    String linea = c.readStringUntil('\n');
    if (linea.length() <= 1) break;
  }

  if (peticion.indexOf("GET /on") >= 0 || peticion.indexOf("GET /off") >= 0) {
    c.println("HTTP/1.1 303 See Other");      // vuelve a la pagina principal
    c.println("Location: /");
    c.println("Connection: close");
    c.println();
  } else {
    enviarPagina(c);
  }
  delay(5);
  c.stop();
}
