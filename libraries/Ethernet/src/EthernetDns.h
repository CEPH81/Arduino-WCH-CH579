/*
 * EthernetDns.h - Cliente DNS minimo (registros A) sobre EthernetUDP.
 */
#ifndef ETHERNET_CH579_DNS_H
#define ETHERNET_CH579_DNS_H

#include <Arduino.h>
#include "api/IPAddress.h"

class DNSClient
{
public:
    void begin(const IPAddress &dnsServer) { _server = dnsServer; }
    /* 1 = resuelto, 0 = no encontrado / sin respuesta, -1 = error de parametros */
    int getHostByName(const char *host, IPAddress &result, unsigned long timeout = 3000);

private:
    IPAddress _server;
};

#endif
