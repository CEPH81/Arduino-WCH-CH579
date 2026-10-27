/*
 * MQTT_PubSub - Publica A0 y millis() cada 5 s en un broker MQTT y
 * enciende/apaga LED2 con mensajes "1"/"0" en el tema ch579m/led.
 *
 * Requiere la libreria PubSubClient (Nick O'Leary):
 *   Herramientas -> Administrar bibliotecas -> buscar "PubSubClient" -> Instalar
 *
 * Broker: uno en su PC (Mosquitto) o uno publico de pruebas sin TLS,
 * por ejemplo test.mosquitto.org o broker.hivemq.com (puerto 1883).
 * Para ver los mensajes en el PC:
 *   mosquitto_sub -h <broker> -t "ch579m/#" -v
 *   mosquitto_pub -h <broker> -t ch579m/led -m 1
 */
#include <Ethernet.h>
#include <PubSubClient.h>

const char *BROKER   = "test.mosquitto.org";   // o la IP de su PC: "192.168.1.100"
const uint16_t PUERTO = 1883;

EthernetClient red;
PubSubClient mqtt(red);
char idCliente[24];
unsigned long ultimo = 0;

void alRecibir(char *tema, byte *datos, unsigned int len) {
  Serial.print("Mensaje en ");
  Serial.print(tema);
  Serial.print(": ");
  Serial.write(datos, len);
  Serial.println();
  if (strcmp(tema, "ch579m/led") == 0 && len > 0) {
    digitalWrite(LED_BUILTIN, datos[0] == '1' ? HIGH : LOW);
    mqtt.publish("ch579m/led/estado", datos[0] == '1' ? "encendido" : "apagado", true);
  }
}

void conectarMqtt() {
  while (!mqtt.connected()) {
    Serial.print("Conectando al broker ");
    Serial.print(BROKER);
    Serial.print(" ... ");
    if (mqtt.connect(idCliente, "ch579m/estado", 0, true, "desconectado")) {
      Serial.println("OK");
      mqtt.publish("ch579m/estado", "conectado", true);
      mqtt.subscribe("ch579m/led");
    } else {
      Serial.print("fallo, codigo ");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  if (!Ethernet.begin()) {
    Serial.println("DHCP fallo");
    while (true) delay(1000);
  }
  Serial.print("Mi IP: ");
  Serial.println(Ethernet.localIP());

  /* ID de cliente unico a partir de la MAC */
  uint8_t mac[6];
  Ethernet.MACAddress(mac);
  snprintf(idCliente, sizeof(idCliente), "ch579m-%02X%02X%02X", mac[3], mac[4], mac[5]);

  mqtt.setServer(BROKER, PUERTO);
  mqtt.setCallback(alRecibir);
}

void loop() {
  if (!mqtt.connected()) conectarMqtt();
  mqtt.loop();

  if (millis() - ultimo > 5000) {
    ultimo = millis();
    char txt[16];
    snprintf(txt, sizeof(txt), "%d", analogReadMillivolts(A0));
    mqtt.publish("ch579m/a0_mv", txt);
    snprintf(txt, sizeof(txt), "%lu", millis() / 1000);
    mqtt.publish("ch579m/uptime_s", txt);
    Serial.println("Publicado A0 y uptime");
  }
}
