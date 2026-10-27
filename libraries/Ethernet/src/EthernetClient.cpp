#include "Ethernet.h"
#include "ch579_net.h"

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
    if (_id >= 0) stop();
    uint8_t a[4] = { ip[0], ip[1], ip[2], ip[3] };
    net_poll();
    _id = net_tcp_connect(a, port);
    if (_id < 0) return 0;

    unsigned long t0 = millis();
    while (!net_tcp_connected(_id)) {
        net_poll();
        if (net_tcp_peer_closed(_id) || millis() - t0 >= _timeout) {
            net_close(_id);
            _id = -1;
            return 0;
        }
    }
    return 1;
}

int EthernetClient::connect(const char *host, uint16_t port)
{
    IPAddress ip;
    if (!Ethernet.hostByName(host, ip)) return 0;
    return connect(ip, port);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
    if (_id < 0) return 0;
    size_t done = 0;
    unsigned long t0 = millis();
    while (done < size) {
        net_poll();
        if (!net_tcp_connected(_id)) break;
        int n = net_tcp_send(_id, buf + done, (int)(size - done));
        if (n > 0) {
            done += (size_t)n;
            t0 = millis();
        } else if (millis() - t0 >= _timeout) {
            break;                          /* la otra punta no acepta datos */
        }
    }
    if (done < size) setWriteError();
    return done;
}

int EthernetClient::available()
{
    if (_id < 0) return 0;
    net_poll();
    return net_rx_available(_id);
}

int EthernetClient::read()
{
    uint8_t b;
    return read(&b, 1) == 1 ? b : -1;
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
    if (_id < 0) return -1;
    net_poll();
    int n = net_rx_read(_id, buf, (int)size);
    return n > 0 ? n : -1;
}

int EthernetClient::peek()
{
    if (_id < 0) return -1;
    net_poll();
    return net_rx_peek(_id);
}

void EthernetClient::stop()
{
    if (_id < 0) return;
    net_close(_id);
    net_poll();
    _id = -1;
}

/* Conectado, o desconectado pero con datos aun sin leer (como en Arduino) */
uint8_t EthernetClient::connected()
{
    if (_id < 0) return 0;
    net_poll();
    uint8_t t = net_sock_type(_id);
    if (t != NET_TCP_CLIENT && t != NET_TCP_ACCEPTED) return 0;
    return net_tcp_connected(_id) || net_rx_available(_id) > 0;
}

IPAddress EthernetClient::remoteIP()
{
    uint8_t a[4]; uint16_t p;
    net_remote(_id, a, &p);
    return IPAddress(a[0], a[1], a[2], a[3]);
}

uint16_t EthernetClient::remotePort()
{
    uint8_t a[4]; uint16_t p;
    net_remote(_id, a, &p);
    return p;
}

uint16_t EthernetClient::localPort() { return net_local_port(_id); }
