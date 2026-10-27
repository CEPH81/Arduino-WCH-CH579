/*
 * ch579_net.c - Capa en C sobre la pila TCP/IP de WCH (CH57xNET.LIB).
 *
 * Basado en los ejemplos oficiales de WCH (TCP_Client, TCP_Server,
 * UDP_Server, DHCP_Client). Diferencias:
 *   - El temporizador de 10 ms de la pila sale del SysTick del core
 *     (ch579_systick_hook), asi TMR0 queda libre para el usuario.
 *   - Los datos recibidos se copian enseguida a un buffer propio por socket:
 *     cuando el otro extremo cierra, la pila libera el socket, pero lo ya
 *     recibido sigue disponible para read().
 */
#include "Arduino.h"

/* Configuracion de la pila (antes de incluir CH57xNET.h) */
#define CH57xNET_MAX_SOCKET_NUM   6
#define CH57xNET_NUM_TCP          5
#define CH57xNET_NUM_TCP_LISTEN   2
#define CH57xNET_NUM_UDP          4
#include "CH57xNET.h"
#include "ch579_net.h"

#if CH57xNET_MAX_SOCKET_NUM != NET_MAX_SOCKETS
#error "NET_MAX_SOCKETS debe coincidir con CH57xNET_MAX_SOCKET_NUM"
#endif

/* ------------------------------------------------- memoria que pide la pila
 * Los nombres y tamanos los exige CH57xNET.LIB (ver CH57xNET.h). */
__attribute__((aligned(16))) UINT8 CH57xMACRxDesBuf[RX_QUEUE_ENTRIES * 16];
__attribute__((aligned(4)))  UINT8 CH57xMACRxBuf[RX_QUEUE_ENTRIES * RX_BUF_SIZE];
__attribute__((aligned(4)))  SOCK_INF SocketInf[CH57xNET_MAX_SOCKET_NUM];
UINT16 MemNum[8] = {
    CH57xNET_NUM_IPRAW, CH57xNET_NUM_UDP, CH57xNET_NUM_TCP, CH57xNET_NUM_TCP_LISTEN,
    CH57xNET_NUM_TCP_SEG, CH57xNET_NUM_IP_REASSDATA, CH57xNET_NUM_PBUF, CH57xNET_NUM_POOL_BUF
};
UINT16 MemSize[8] = {
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IPRAW_PCB),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_UDP_PCB),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB_LISTEN),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_SEG),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IP_REASSDATA),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(0),
    CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_POOL_BUF)
};
__attribute__((aligned(4))) UINT8 Memp_Memory[CH57xNET_MEMP_SIZE];
__attribute__((aligned(4))) UINT8 Mem_Heap_Memory[CH57xNET_RAM_HEAP_SIZE];
__attribute__((aligned(4))) UINT8 Mem_ArpTable[CH57xNET_RAM_ARP_TABLE_SIZE];
UINT8 MACAddr[6];                      /* la pila lee la MAC de esta variable */

/* ---------------------------------------------------------- estado propio */
typedef struct {
    uint8_t  type;
    volatile uint8_t connected;
    uint8_t  peer_closed;
    uint8_t  accepted_new;
    uint16_t local_port;
    uint8_t  remote_ip[4];
    uint16_t remote_port;
    uint16_t udp_left;                 /* bytes que quedan del paquete UDP actual */
    uint16_t head, tail;               /* buffer circular propio */
    uint8_t  rx[NET_APP_RXBUF];
} net_sock_t;

static net_sock_t s_sock[NET_MAX_SOCKETS];
static __attribute__((aligned(4))) uint8_t s_librx[NET_MAX_SOCKETS][NET_LIB_RXBUF];

static volatile bool s_ready = false;
static volatile uint8_t s_tick = 0;
static bool     s_in_poll = false;
static uint16_t s_next_port = 49152;

static volatile int s_dhcp_state = -1;
static uint8_t s_dhcp_res[20];

/* ------------------------------------------------- interrupciones y tiempo */
void ETH_IRQHandler(void)
{
    CH57xNET_ETHIsr();
}

/* Llamada cada 1 ms desde el SysTick del core */
void ch579_systick_hook(void)
{
    if (!s_ready) return;
    if (++s_tick >= CH57xNETTIMEPERIOD) {
        s_tick = 0;
        CH57xNET_TimeIsr(CH57xNETTIMEPERIOD);
    }
}

/* ------------------------------------------------- buffer circular propio */
static inline int ring_used(const net_sock_t *s) { return (uint16_t)(s->head - s->tail) % NET_APP_RXBUF; }
static inline int ring_free(const net_sock_t *s) { return NET_APP_RXBUF - 1 - ring_used(s); }

static void ring_put(net_sock_t *s, const uint8_t *d, int n)
{
    for (int i = 0; i < n; i++) {
        s->rx[s->head] = d[i];
        s->head = (uint16_t)((s->head + 1) % NET_APP_RXBUF);
    }
}

static int ring_get(net_sock_t *s, uint8_t *d, int n)
{
    int k = 0;
    while (k < n && s->tail != s->head) {
        if (d) d[k] = s->rx[s->tail];
        s->tail = (uint16_t)((s->tail + 1) % NET_APP_RXBUF);
        k++;
    }
    return k;
}

static void sock_reset(int id)
{
    net_sock_t *s = &s_sock[id];
    s->type = NET_FREE;
    s->connected = 0;
    s->peer_closed = 0;
    s->accepted_new = 0;
    s->udp_left = 0;
    s->head = s->tail = 0;
}

/* Pasa a nuestro buffer lo que la pila tenga recibido para este socket */
static void tcp_drain(int id)
{
    net_sock_t *s = &s_sock[id];
    uint8_t tmp[64];
    while (1) {
        UINT32 avail = CH57xNET_SocketRecvLen((UINT8)id, NULL);
        int room = ring_free(s);
        if (avail == 0 || room == 0) break;
        UINT32 n = avail;
        if (n > (UINT32)room) n = (UINT32)room;
        if (n > sizeof(tmp)) n = sizeof(tmp);
        if (CH57xNET_SocketRecv((UINT8)id, tmp, &n) != CH57xNET_ERR_SUCCESS || n == 0) break;
        ring_put(s, tmp, (int)n);
    }
}

/* UDP: la pila entrega cada paquete con la funcion del socket. Se usa una
 * funcion distinta por socket (udp_cb0..5) para saber el indice sin depender
 * del puntero 'inf' que pasa la pila. */
static void udp_store(int id, UINT32 ipaddr, UINT16 port, UINT8 *buf, UINT32 len)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) return;
    net_sock_t *s = &s_sock[id];
    if (s->type != NET_UDP) return;
    if ((int)len + 8 > ring_free(s)) return;            /* sin sitio: se descarta */
    uint8_t hdr[8] = {
        (uint8_t)len, (uint8_t)(len >> 8),
        (uint8_t)ipaddr, (uint8_t)(ipaddr >> 8), (uint8_t)(ipaddr >> 16), (uint8_t)(ipaddr >> 24),
        (uint8_t)port, (uint8_t)(port >> 8)
    };
    ring_put(s, hdr, 8);
    ring_put(s, buf, (int)len);
}

#define UDP_CB(n) static void udp_cb##n(struct _SCOK_INF *inf, UINT32 ip, UINT16 port, UINT8 *buf, UINT32 len) \
    { (void)inf; udp_store(n, ip, port, buf, len); }
UDP_CB(0) UDP_CB(1) UDP_CB(2) UDP_CB(3) UDP_CB(4) UDP_CB(5)
typedef void (*udp_cb_t)(struct _SCOK_INF *, UINT32, UINT16, UINT8 *, UINT32);
static const udp_cb_t s_udp_cb[NET_MAX_SOCKETS] = { udp_cb0, udp_cb1, udp_cb2, udp_cb3, udp_cb4, udp_cb5 };

/* Funcion inicial (antes de conocer el indice): usa el de 'inf' */
static void udp_recv_cb(struct _SCOK_INF *inf, UINT32 ip, UINT16 port, UINT8 *buf, UINT32 len)
{
    udp_store((int)inf->SockIndex, ip, port, buf, len);
}

/* ------------------------------------------------- bucle de la pila */
static void handle_socket_int(int id, uint8_t st)
{
    net_sock_t *s = &s_sock[id];

    if (st & SINT_STAT_CONNECT) {
        if (s->type == NET_FREE || s->type == NET_TCP_ACCEPTED || s->peer_closed) {
            /* Conexion entrante aceptada por un socket en escucha */
            sock_reset(id);
            s->type = NET_TCP_ACCEPTED;
            s->accepted_new = 1;
            CH57xNET_ModifyRecvBuf((UINT8)id, (UINT32)s_librx[id], NET_LIB_RXBUF);
        }
        s->local_port  = (uint16_t)SocketInf[id].SourPort;
        s->remote_port = (uint16_t)SocketInf[id].DesPort;
        memcpy(s->remote_ip, SocketInf[id].IPAddr, 4);
        s->connected = 1;
    }
    if ((st & SINT_STAT_RECV) && s->type != NET_UDP && s->type != NET_FREE) {
        tcp_drain(id);
    }
    if (st & (SINT_STAT_DISCONNECT | SINT_STAT_TIM_OUT)) {
        if (s->type == NET_TCP_CLIENT || s->type == NET_TCP_ACCEPTED) {
            tcp_drain(id);
            s->connected = 0;
            s->peer_closed = 1;        /* la pila ya libero el socket */
        }
    }
}

void net_poll(void)
{
    if (!s_ready || s_in_poll) return;
    s_in_poll = true;

    CH57xNET_MainTask();
    if (CH57xNET_QueryGlobalInt()) {
        uint8_t g = CH57xNET_GetGlobalInt();
        if (g & GINT_STAT_SOCKET) {
            for (int i = 0; i < NET_MAX_SOCKETS; i++) {
                uint8_t st = CH57xNET_GetSocketInt((UINT8)i);
                if (st) handle_socket_int(i, st);
            }
        }
    }
    /* Datos que quedaron en la pila porque nuestro buffer estaba lleno */
    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        net_sock_t *s = &s_sock[i];
        if ((s->type == NET_TCP_CLIENT || s->type == NET_TCP_ACCEPTED) && s->connected)
            tcp_drain(i);
    }
    s_in_poll = false;
}

/* ------------------------------------------------- inicializacion */
int net_init(const uint8_t mac[6], const uint8_t ip[4], const uint8_t gw[4], const uint8_t mask[4])
{
    static const uint8_t zero[4] = { 0, 0, 0, 0 };
    if (CH57xNET_GetVer() != CH57xNET_LIB_VER) return 0xFC;

    /* La pila y el PHY usan el PLL */
    PWR_UnitModCfg(ENABLE, UNIT_SYS_PLL);
    delay(3);

    if (mac) memcpy(MACAddr, mac, 6);
    else     GetMACAddress(MACAddr);      /* MAC unica grabada de fabrica */

    for (int i = 0; i < NET_MAX_SOCKETS; i++) sock_reset(i);

    CH57xNETConfig = LIB_CFG_VALUE;
    struct _CH57x_CFG cfg;
    cfg.RxBufSize   = RX_BUF_SIZE;
    cfg.TCPMss      = CH57xNET_TCP_MSS;
    cfg.HeapSize    = CH57x_MEM_HEAP_SIZE;
    cfg.ARPTableNum = CH57xNET_NUM_ARP_TABLE;
    cfg.MiscConfig0 = CH57xNET_MISC_CONFIG0;
    CH57xNET_ConfigLIB(&cfg);

    uint8_t r = CH57xNET_Init(ip ? ip : zero, gw ? gw : zero, mask ? mask : zero, MACAddr);
    if (r != CH57xNET_ERR_SUCCESS) return r;

    struct _KEEP_CFG kl = { 20000, 10000, 5 };   /* keep-alive como en los ejemplos de WCH */
    CH57xNET_ConfigKeepLive(&kl);

    s_ready = true;
    NVIC_EnableIRQ(ETH_IRQn);
    return 0;
}

bool    net_ready(void)      { return s_ready; }
uint8_t net_phy_status(void) { return (uint8_t)CH57xInf.PHYStat; }
bool    net_link_up(void)    { return s_ready && CH57xInf.PHYStat >= PHY_10M_FLL; }

void net_get_config(uint8_t ip[4], uint8_t gw[4], uint8_t mask[4])
{
    if (ip)   memcpy(ip,   CH57xInf.IPAddr, 4);
    if (gw)   memcpy(gw,   CH57xInf.GWIPAddr, 4);
    if (mask) memcpy(mask, CH57xInf.MASKAddr, 4);
}

void net_get_mac(uint8_t mac[6]) { memcpy(mac, MACAddr, 6); }

/* ------------------------------------------------- DHCP */
static UINT8 dhcp_cb(UINT8 status, void *arg)
{
    if (status == 0 && arg) {
        memcpy(s_dhcp_res, arg, 20);   /* IP, puerta de enlace, mascara, DNS1, DNS2 */
        s_dhcp_state = 1;
    } else {
        s_dhcp_state = -1;
    }
    return 0;
}

int net_dhcp_start(void)
{
    s_dhcp_state = 0;
    if (CH57xNET_DHCPStart(dhcp_cb) != CH57xNET_ERR_SUCCESS) {
        s_dhcp_state = -1;
        return -1;
    }
    return 0;
}

int net_dhcp_state(void)         { return s_dhcp_state; }
const uint8_t *net_dhcp_dns(void){ return &s_dhcp_res[12]; }

/* ------------------------------------------------- sockets */
static int sock_create(uint8_t proto, const uint8_t ip[4], uint16_t dport, uint16_t sport, bool udp)
{
    SOCK_INF inf;
    memset(&inf, 0, sizeof(inf));
    memcpy(inf.IPAddr, ip, 4);
    inf.DesPort   = dport;
    inf.SourPort  = sport;
    inf.ProtoType = proto;
    UINT8 id = 0xFF;

    /* La pila elige el indice; el buffer se ajusta despues con ModifyRecvBuf */
    inf.RecvStartPoint = (UINT32)s_librx[0];
    inf.RecvBufLen     = NET_LIB_RXBUF;
    if (udp) inf.AppCallBack = udp_recv_cb;
    if (CH57xNET_SocketCreat(&id, &inf) != CH57xNET_ERR_SUCCESS || id >= NET_MAX_SOCKETS) return -1;
    CH57xNET_ModifyRecvBuf(id, (UINT32)s_librx[id], NET_LIB_RXBUF);
    if (udp) SocketInf[id].AppCallBack = s_udp_cb[id];   /* funcion propia del socket */
    sock_reset(id);
    s_sock[id].local_port = sport;
    return id;
}

static uint16_t ephemeral_port(void)
{
    if (++s_next_port < 49152) s_next_port = 49152;
    return s_next_port;
}

int net_tcp_connect(const uint8_t ip[4], uint16_t port)
{
    if (!s_ready) return -1;
    int id = sock_create(PROTO_TYPE_TCP, ip, port, ephemeral_port(), false);
    if (id < 0) return -1;
    s_sock[id].type = NET_TCP_CLIENT;
    memcpy(s_sock[id].remote_ip, ip, 4);
    s_sock[id].remote_port = port;
    CH57xNET_SocketSetKeepLive((UINT8)id, 1);
    if (CH57xNET_SocketConnect((UINT8)id) != CH57xNET_ERR_SUCCESS) {
        CH57xNET_SocketClose((UINT8)id, TCP_CLOSE_ABANDON);
        sock_reset(id);
        return -1;
    }
    return id;
}

int net_tcp_listen(uint16_t port)
{
    static const uint8_t any[4] = { 0, 0, 0, 0 };
    if (!s_ready) return -1;
    int id = sock_create(PROTO_TYPE_TCP, any, 0, port, false);
    if (id < 0) return -1;
    if (CH57xNET_SocketListen((UINT8)id) != CH57xNET_ERR_SUCCESS) {
        CH57xNET_SocketClose((UINT8)id, TCP_CLOSE_ABANDON);
        sock_reset(id);
        return -1;
    }
    s_sock[id].type = NET_TCP_LISTEN;
    return id;
}

int net_udp_open(uint16_t port)
{
    /* Como el ejemplo UDP_Server de WCH: destino 255.255.255.255 = acepta
     * paquetes de cualquier IP; cada envio indica su destino con SendTo */
    static const uint8_t bcast[4] = { 255, 255, 255, 255 };
    if (!s_ready) return -1;
    if (port == 0) port = ephemeral_port();
    int id = sock_create(PROTO_TYPE_UDP, bcast, port, port, true);
    if (id < 0) return -1;
    s_sock[id].type = NET_UDP;
    return id;
}

void net_close(int id)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) return;
    net_sock_t *s = &s_sock[id];
    if (s->type == NET_FREE) return;
    if (s->type == NET_UDP || s->type == NET_TCP_LISTEN) {
        CH57xNET_SocketClose((UINT8)id, TCP_CLOSE_NORMAL);
    } else if (!s->peer_closed) {
        CH57xNET_SocketClose((UINT8)id, TCP_CLOSE_NORMAL);   /* FIN, cierre ordenado */
    }
    sock_reset(id);
}

uint8_t  net_sock_type(int id)       { return (id >= 0 && id < NET_MAX_SOCKETS) ? s_sock[id].type : NET_FREE; }
bool     net_tcp_connected(int id)   { return id >= 0 && id < NET_MAX_SOCKETS && s_sock[id].connected; }
bool     net_tcp_peer_closed(int id) { return id >= 0 && id < NET_MAX_SOCKETS && s_sock[id].peer_closed; }
uint16_t net_local_port(int id)      { return (id >= 0 && id < NET_MAX_SOCKETS) ? s_sock[id].local_port : 0; }

bool net_take_accepted(int id)
{
    if (id < 0 || id >= NET_MAX_SOCKETS || !s_sock[id].accepted_new) return false;
    s_sock[id].accepted_new = 0;
    return true;
}

void net_remote(int id, uint8_t ip[4], uint16_t *port)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) { memset(ip, 0, 4); *port = 0; return; }
    memcpy(ip, s_sock[id].remote_ip, 4);
    *port = s_sock[id].remote_port;
}

int net_rx_available(int id)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) return 0;
    net_sock_t *s = &s_sock[id];
    if (s->type == NET_UDP) return s->udp_left;
    return ring_used(s);
}

int net_rx_read(int id, uint8_t *buf, int len)
{
    if (id < 0 || id >= NET_MAX_SOCKETS || len <= 0) return 0;
    net_sock_t *s = &s_sock[id];
    if (s->type == NET_UDP) {
        if (len > s->udp_left) len = s->udp_left;
        int n = ring_get(s, buf, len);
        s->udp_left = (uint16_t)(s->udp_left - n);
        return n;
    }
    return ring_get(s, buf, len);
}

int net_rx_peek(int id)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) return -1;
    net_sock_t *s = &s_sock[id];
    if (s->head == s->tail) return -1;
    if (s->type == NET_UDP && s->udp_left == 0) return -1;
    return s->rx[s->tail];
}

int net_tcp_send(int id, const uint8_t *buf, int len)
{
    if (id < 0 || id >= NET_MAX_SOCKETS || !s_sock[id].connected || len <= 0) return 0;
    UINT32 n = (UINT32)len;
    if (CH57xNET_SocketSend((UINT8)id, (UINT8 *)buf, &n) != CH57xNET_ERR_SUCCESS) return 0;
    return (int)n;
}

int net_udp_next(int id, uint8_t ip[4], uint16_t *port)
{
    if (id < 0 || id >= NET_MAX_SOCKETS) return 0;
    net_sock_t *s = &s_sock[id];
    if (s->type != NET_UDP) return 0;
    if (s->udp_left) { ring_get(s, NULL, s->udp_left); s->udp_left = 0; }   /* descarta el resto */
    if (ring_used(s) < 8) return 0;
    uint8_t hdr[8];
    ring_get(s, hdr, 8);
    s->udp_left = (uint16_t)(hdr[0] | (hdr[1] << 8));
    memcpy(ip, &hdr[2], 4);
    *port = (uint16_t)(hdr[6] | (hdr[7] << 8));
    return s->udp_left;
}

static volatile uint8_t net_last_err;
int net_udp_sendto(int id, const uint8_t *buf, int len, const uint8_t ip[4], uint16_t port)
{
    if (id < 0 || id >= NET_MAX_SOCKETS || s_sock[id].type != NET_UDP || len <= 0) return 0;
    UINT32 n = 0;
    /* El primer envio a una IP sin entrada ARP falla (la pila solo lanza la
     * peticion ARP): se reintenta mientras llega la respuesta (hasta ~0,5 s). */
    uint32_t t0 = millis();
    for (;;) {
        n = (UINT32)len;
        net_last_err = CH57xNET_SocketUdpSendTo((UINT8)id, (UINT8 *)buf, &n, (UINT8 *)ip, port);
        if (net_last_err == CH57xNET_ERR_SUCCESS) break;
        if (millis() - t0 > 500) return 0;
        net_poll();
    }
    return (int)n;
}

/* delay() llama a yield(): asi la pila sigue atendida mientras el sketch espera */
void yield(void)
{
    net_poll();
}
