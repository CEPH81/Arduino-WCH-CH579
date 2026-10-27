/*
 * 19_SerialUSB - Puerto serie virtual por el USB nativo del CH579.
 *
 * Conecte el SEGUNDO micro-USB de la placa (el del CH579, PB10/PB11),
 * no el del CH340. En Windows 10/11 aparece un puerto COM nuevo
 * ("Dispositivo serie USB") sin instalar drivers.
 *
 * Abra ese puerto en el Monitor Serie (la velocidad no importa en USB):
 *   - LED2 parpadea rapido mientras espera a que el PC abra el puerto
 *   - luego envia un saludo y hace eco de lo que escriba
 *   - el comando "info" muestra el estado del puerto
 *
 * Use el reloj de cristal (menu Reloj del sistema = HSE o PLL).
 */
String linea;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  SerialUSB.begin();

  /* Espera a que el PC abra el puerto (DTR activo) */
  while (!SerialUSB) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  SerialUSB.println("Hola desde el USB nativo del CH579M");
  SerialUSB.println("Escriba algo (o 'info') y pulse Enter:");
}

void loop() {
  while (SerialUSB.available()) {
    char c = (char)SerialUSB.read();
    if (c == '\r' || c == '\n') {
      if (linea.length() == 0) continue;
      if (linea == "info") {
        SerialUSB.print("Baudios elegidos en el PC: ");
        SerialUSB.println(SerialUSB.baud());
        SerialUSB.print("DTR="); SerialUSB.print(SerialUSB.dtr());
        SerialUSB.print(" RTS="); SerialUSB.println(SerialUSB.rts());
        SerialUSB.print("millis = "); SerialUSB.println(millis());
      } else {
        SerialUSB.print("Eco: ");
        SerialUSB.println(linea);
      }
      linea = "";
    } else {
      linea += c;
    }
  }

  /* Si el PC cierra el puerto, LED2 se apaga */
  digitalWrite(LED_BUILTIN, SerialUSB ? HIGH : LOW);
}
