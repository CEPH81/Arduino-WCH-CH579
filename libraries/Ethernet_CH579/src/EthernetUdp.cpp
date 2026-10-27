#include "Ethernet.h"
#include "ch579_net.h"

uint8_t EthernetUDP::begin(uint16_t port)
{
    if (_id >= 0) stop();
    net_poll();
    _id = net_udp_open(port);
    return _id >= 0 ? 1 : 0;
}

void EthernetUDP::stop()
{
    if (_id < 0) return;
    net_close(_id);
    _id = -1;
}

uint16_t EthernetUDP::localPort() { return net_local_port(_id); }

int EthernetUDP::beginPacket(IPAddress ip, uint16_t port)
{
    if (_id < 0 && !begin(0)) return 0;        /* socket efimero para solo enviar */
    _rip = ip;
    _rport = port;
    _txLen = 0;
    return 1;
}

int EthernetUDP::beginPacket(const char *host, uint16_t port)
{
    IPAddress ip;
    if (!Ethernet.hostByName(host, ip)) return 0;
    return beginPacket(ip, port);
}

size_t EthernetUDP::write(const uint8_t *buf, size_t size)
{
    size_t room = UDP_TX_PACKET_MAX_SIZE - _txLen;
    if (size > room) size = room;
    memcpy(_tx + _txLen, buf, size);
    _txLen = (uint16_t)(_txLen + size);
    return size;
}

int EthernetUDP::endPacket()
{
    if (_id < 0) return 0;
    uint8_t a[4] = { _rip[0], _rip[1], _rip[2], _rip[3] };
    int n = net_udp_sendto(_id, _tx, _txLen, a, _rport);
    _txLen = 0;
    net_poll();
    return n > 0 ? 1 : 0;
}

int EthernetUDP::parsePacket()
{
    if (_id < 0) return 0;
    net_poll();
    uint8_t a[4]; uint16_t p;
    int n = net_udp_next(_id, a, &p);
    if (n > 0) {
        _remoteIP = IPAddress(a[0], a[1], a[2], a[3]);
        _remotePort = p;
    }
    return n;
}

int EthernetUDP::available() { return _id < 0 ? 0 : net_rx_available(_id); }

int EthernetUDP::read()
{
    uint8_t b;
    return read(&b, 1) == 1 ? b : -1;
}

int EthernetUDP::read(unsigned char *buf, size_t len)
{
    if (_id < 0) return -1;
    int n = net_rx_read(_id, buf, (int)len);
    return n > 0 ? n : -1;
}

int EthernetUDP::peek() { return _id < 0 ? -1 : net_rx_peek(_id); }
