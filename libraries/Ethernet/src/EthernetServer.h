/*
 * EthernetServer.h - Servidor TCP (Arduino Server) sobre el CH579.
 *
 * available() devuelve un cliente que tenga datos para leer; accept()
 * devuelve cada conexion nueva una sola vez (aunque aun no haya datos).
 */
#ifndef ETHERNET_CH579_SERVER_H
#define ETHERNET_CH579_SERVER_H

#include <Arduino.h>
#include "api/Server.h"
#include "EthernetClient.h"

class EthernetServer : public arduino::Server
{
public:
    explicit EthernetServer(uint16_t port) : _port(port), _listen(-1) { }
    void begin() override;
    void begin(uint16_t port) { _port = port; begin(); }
    EthernetClient available();
    EthernetClient accept();
    size_t write(uint8_t b) override { return write(&b, 1); }
    size_t write(const uint8_t *buf, size_t size) override;   /* a todos los clientes */
    using Print::write;
    operator bool() { return _listen >= 0; }

private:
    uint16_t _port;
    int      _listen;
};

#endif
