/*
 * ch579_net.h - Capa en C sobre la pila TCP/IP de WCH (CH57xNET.LIB).
 * Uso interno de la libreria Ethernet_CH579 (las clases de Arduino la usan).
 */
#ifndef CH579_NET_H
#define CH579_NET_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_MAX_SOCKETS   6       /* sockets de la pila (TCP + UDP + escucha) */
#define NET_LIB_RXBUF     768     /* buffer de recepcion que usa la pila (ventana TCP) */
#define NET_APP_RXBUF     512     /* buffer propio por socket */

enum {
    NET_FREE = 0,
    NET_TCP_CLIENT,
    NET_TCP_LISTEN,
    NET_TCP_ACCEPTED,
    NET_UDP
};

typedef void (*net_udp_cb_t)(void);

/* --- inicializacion y estado --- */
int      net_init(const uint8_t mac[6], const uint8_t ip[4], const uint8_t gw[4], const uint8_t mask[4]);
bool     net_ready(void);
void     net_poll(void);
bool     net_link_up(void);
uint8_t  net_phy_status(void);
void     net_get_config(uint8_t ip[4], uint8_t gw[4], uint8_t mask[4]);
void     net_get_mac(uint8_t mac[6]);
uint16_t net_phy_read(uint8_t reg);      /* registro MII del PHY (diagnostico) */

/* --- DHCP --- */
int      net_dhcp_start(void);
int      net_dhcp_state(void);           /* 0 = en curso, 1 = ok, -1 = fallo */
const uint8_t *net_dhcp_dns(void);       /* DNS que dio el servidor DHCP */

/* --- sockets --- */
int      net_tcp_connect(const uint8_t ip[4], uint16_t port);   /* id o -1 */
int      net_tcp_listen(uint16_t port);                         /* id o -1 */
int      net_udp_open(uint16_t port);                           /* id o -1 */
void     net_close(int id);
uint8_t  net_sock_type(int id);
bool     net_tcp_connected(int id);
bool     net_tcp_peer_closed(int id);
bool     net_take_accepted(int id);      /* true una vez tras una conexion entrante */
uint16_t net_local_port(int id);
void     net_remote(int id, uint8_t ip[4], uint16_t *port);

int      net_rx_available(int id);
int      net_rx_read(int id, uint8_t *buf, int len);
int      net_rx_peek(int id);
int      net_tcp_send(int id, const uint8_t *buf, int len);     /* bytes aceptados */

/* UDP: paquetes completos */
int      net_udp_next(int id, uint8_t ip[4], uint16_t *port);   /* tamano del siguiente paquete o 0 */
int      net_udp_sendto(int id, const uint8_t *buf, int len, const uint8_t ip[4], uint16_t port);

#ifdef __cplusplus
}
#endif

#endif
