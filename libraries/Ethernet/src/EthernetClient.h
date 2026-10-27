/*
 * EthernetClient.h - Cliente TCP (Arduino Client) sobre el CH579.
 */
#ifndef ETHERNET_CH579_CLIENT_H
#define ETHERNET_CH579_CLIENT_H

#include <Arduino.h>
#include "api/Client.h"

class EthernetServer;

class EthernetClient : public arduino::Client
{
public:
    EthernetClient() : _id(-1), _timeout(5000) { }
    explicit EthernetClient(int id) : _id(id), _timeout(5000) { }

    int connect(IPAddress ip, uint16_t port) override;
    int connect(const char *host, uint16_t port) override;
    size_t write(uint8_t b) override { return write(&b, 1); }
    size_t write(const uint8_t *buf, size_t size) override;
    using Print::write;
    int available() override;
    int read() override;
    int read(uint8_t *buf, size_t size) override;
    int peek() override;
    void flush() override { }
    void stop() override;
    uint8_t connected() override;
    operator bool() override { return _id >= 0; }

    bool operator==(const EthernetClient &rhs) const { return _id == rhs._id; }
    bool operator!=(const EthernetClient &rhs) const { return _id != rhs._id; }

    IPAddress remoteIP();
    uint16_t  remotePort();
    uint16_t  localPort();
    void setConnectionTimeout(uint16_t ms) { _timeout = ms; }
    int  getSocketNumber() const { return _id; }

private:
    int      _id;
    uint16_t _timeout;
};

#endif
