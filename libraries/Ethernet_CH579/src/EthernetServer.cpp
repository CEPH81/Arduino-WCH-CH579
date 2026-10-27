#include "Ethernet.h"
#include "ch579_net.h"

void EthernetServer::begin()
{
    net_poll();
    if (_listen >= 0) return;
    _listen = net_tcp_listen(_port);
}

/* Cliente con datos pendientes (o cerrado con datos sin leer) */
EthernetClient EthernetServer::available()
{
    net_poll();
    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (net_sock_type(i) == NET_TCP_ACCEPTED && net_local_port(i) == _port && net_rx_available(i) > 0) {
            net_take_accepted(i);
            return EthernetClient(i);
        }
    }
    return EthernetClient();
}

/* Cada conexion nueva una sola vez */
EthernetClient EthernetServer::accept()
{
    net_poll();
    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (net_sock_type(i) == NET_TCP_ACCEPTED && net_local_port(i) == _port && net_take_accepted(i)) {
            return EthernetClient(i);
        }
    }
    return EthernetClient();
}

size_t EthernetServer::write(const uint8_t *buf, size_t size)
{
    net_poll();
    size_t n = 0;
    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (net_sock_type(i) == NET_TCP_ACCEPTED && net_local_port(i) == _port && net_tcp_connected(i)) {
            EthernetClient c(i);
            n = c.write(buf, size);
        }
    }
    return n;
}
