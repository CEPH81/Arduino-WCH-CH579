/*
 * EthernetUdp.h - UDP (Arduino UDP) sobre el CH579.
 */
#ifndef ETHERNET_CH579_UDP_H
#define ETHERNET_CH579_UDP_H

#include <Arduino.h>
#include "api/Udp.h"

#ifndef UDP_TX_PACKET_MAX_SIZE
#define UDP_TX_PACKET_MAX_SIZE 512
#endif

class EthernetUDP : public arduino::UDP
{
public:
    EthernetUDP() : _id(-1), _txLen(0), _rport(0), _remotePort(0) { }

    uint8_t begin(uint16_t port) override;
    uint8_t beginMulticast(IPAddress, uint16_t) override { return 0; }   /* no soportado */
    void stop() override;

    int beginPacket(IPAddress ip, uint16_t port) override;
    int beginPacket(const char *host, uint16_t port) override;
    int endPacket() override;
    size_t write(uint8_t b) override { return write(&b, 1); }
    size_t write(const uint8_t *buf, size_t size) override;
    using Print::write;

    int parsePacket() override;
    int available() override;
    int read() override;
    int read(unsigned char *buf, size_t len) override;
    int read(char *buf, size_t len) override { return read((unsigned char *)buf, len); }
    int peek() override;
    void flush() override { }

    IPAddress remoteIP() override { return _remoteIP; }
    uint16_t  remotePort() override { return _remotePort; }
    uint16_t  localPort();

private:
    int       _id;
    uint8_t   _tx[UDP_TX_PACKET_MAX_SIZE];
    uint16_t  _txLen;
    IPAddress _rip;
    uint16_t  _rport;
    IPAddress _remoteIP;
    uint16_t  _remotePort;
};

#endif
