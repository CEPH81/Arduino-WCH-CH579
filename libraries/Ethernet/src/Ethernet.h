/*
 * Ethernet.h - Ethernet del WCH CH579 (MAC + PHY 10BASE-T integrados)
 * con la API de la libreria Ethernet de Arduino.
 *
 * Usa la pila TCP/IP de WCH (CH57xNET.LIB). En la placa CH579M V1.6 el
 * conector RJ45 (HR911105A) va a PB12..PB15. Enlace a 10 Mbit/s.
 *
 *   Ethernet.begin();                       // DHCP, MAC unica de fabrica
 *   Ethernet.begin(IPAddress(192,168,1,200)); // IP fija
 *   Serial.println(Ethernet.localIP());
 *
 * Clases: EthernetClient (TCP cliente), EthernetServer (TCP servidor),
 * EthernetUDP. Funcionan con librerias que usan Client/UDP, como
 * PubSubClient (MQTT).
 *
 * La pila se atiende sola durante delay() y en cada llamada a la libreria;
 * si el sketch hace bucles largos sin delay(), llame a Ethernet.maintain().
 */
#ifndef ETHERNET_CH579_H
#define ETHERNET_CH579_H

#include <Arduino.h>
#include "api/IPAddress.h"
#include "EthernetClient.h"
#include "EthernetServer.h"
#include "EthernetUdp.h"

enum EthernetLinkStatus { Unknown, LinkON, LinkOFF };
enum EthernetHardwareStatus { EthernetNoHardware, EthernetW5100, EthernetW5200, EthernetW5500, EthernetCH579 };

class EthernetClass
{
public:
    /* DHCP. mac = nullptr usa la MAC unica del chip. Devuelve 1 si obtuvo IP. */
    int begin(uint8_t *mac = nullptr, unsigned long timeout = 60000, unsigned long responseTimeout = 4000);

    /* IP fija (dns/gateway/subnet opcionales: por defecto x.x.x.1 y 255.255.255.0) */
    void begin(uint8_t *mac, IPAddress ip);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway);
    void begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet);
    void begin(IPAddress ip) { begin(nullptr, ip); }

    /* IP 169.254.x.y sacada de la MAC (mascara 255.255.0.0): para conectar la
     * placa directo al puerto LAN del PC, sin router (el PC usa la misma red) */
    void beginLinkLocal(uint8_t *mac = nullptr);
    /* DHCP y, si no hay servidor (conexion directa al PC), IP de enlace local.
     * Devuelve 1 = DHCP, 2 = enlace local 169.254.x.y, 0 = sin cable */
    int beginAuto(uint8_t *mac = nullptr, unsigned long timeout = 15000);
    void begin(IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet = IPAddress(255, 255, 255, 0))
        { begin(nullptr, ip, dns, gateway, subnet); }

    int  maintain(void);                     /* atiende la pila; devuelve 0 */
    EthernetLinkStatus     linkStatus(void);
    EthernetHardwareStatus hardwareStatus(void) { return EthernetCH579; }

    IPAddress localIP(void);
    IPAddress subnetMask(void);
    IPAddress gatewayIP(void);
    IPAddress dnsServerIP(void) { return _dns; }
    void      MACAddress(uint8_t *mac);
    void      setDnsServerIP(const IPAddress dns) { _dns = dns; }

    /* Espera hasta 'timeout' ms a que haya enlace (cable conectado) */
    bool waitForLink(unsigned long timeout = 5000);

    /* Resuelve un nombre con el servidor DNS. Devuelve 1 si lo encontro. */
    int hostByName(const char *host, IPAddress &result, unsigned long timeout = 3000);

    /* Mensajes de diagnostico de begin() (enlace, DHCP...): Ethernet.setDebug(Serial) */
    void setDebug(Print &p) { _dbg = &p; }
    /* Estado del PHY segun la pila: 1 = sin cable, 2 = 10M full, 4 = 10M half */
    uint8_t phyStatus(void);
    /* Registro MII del PHY (0 = BMCR, 1 = BMSR: bit 2 = enlace) */
    uint16_t phyRegister(uint8_t reg);

private:
    bool init(uint8_t *mac, const uint8_t ip[4], const uint8_t gw[4], const uint8_t mask[4]);
    IPAddress _dns;
    Print *_dbg = nullptr;
    void dbg(const char *s) { if (_dbg) _dbg->print(s); }
};

extern EthernetClass Ethernet;

#endif
