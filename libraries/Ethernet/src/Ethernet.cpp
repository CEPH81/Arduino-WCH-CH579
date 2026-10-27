#include "Ethernet.h"
#include "ch579_net.h"
#include "EthernetDns.h"

EthernetClass Ethernet;

static void ip_to_bytes(const IPAddress &a, uint8_t b[4]) { for (int i = 0; i < 4; i++) b[i] = a[i]; }

bool EthernetClass::init(uint8_t *mac, const uint8_t ip[4], const uint8_t gw[4], const uint8_t mask[4])
{
    if (net_ready()) return true;      /* la pila solo se inicializa una vez */
    return net_init(mac, ip, gw, mask) == 0;
}

bool EthernetClass::waitForLink(unsigned long timeout)
{
    unsigned long t0 = millis();
    while (!net_link_up()) {
        net_poll();
        if (millis() - t0 >= timeout) return false;
    }
    return true;
}

int EthernetClass::begin(uint8_t *mac, unsigned long timeout, unsigned long responseTimeout)
{
    (void)responseTimeout;
    dbg("[ETH] Iniciando pila TCP/IP... ");
    if (!init(mac, nullptr, nullptr, nullptr)) { dbg("ERROR\r\n"); return 0; }
    dbg("OK\r\n[ETH] Esperando enlace (cable)");

    unsigned long t0 = millis(), tp = t0;
    while (!net_link_up()) {
        net_poll();
        if (millis() - tp >= 1000) { tp = millis(); dbg("."); }
        if (millis() - t0 >= timeout) { dbg(" sin enlace: revise el cable y el router\r\n"); return 0; }
    }
    if (_dbg) { _dbg->print(" OK (PHY="); _dbg->print(net_phy_status()); _dbg->print(")\r\n"); }

    dbg("[ETH] Pidiendo IP por DHCP");
    if (net_dhcp_start() != 0) { dbg(" ERROR al iniciar DHCP\r\n"); return 0; }
    while (net_dhcp_state() == 0) {
        net_poll();
        if (millis() - tp >= 1000) { tp = millis(); dbg("."); }
        if (millis() - t0 >= timeout) { dbg(" sin respuesta del servidor DHCP\r\n"); return 0; }
    }
    if (net_dhcp_state() != 1) { dbg(" fallo\r\n"); return 0; }
    dbg(" OK\r\n");

    const uint8_t *d = net_dhcp_dns();
    _dns = IPAddress(d[0], d[1], d[2], d[3]);
    if (_dns == IPAddress(0, 0, 0, 0)) _dns = gatewayIP();
    return 1;
}

uint8_t  EthernetClass::phyStatus(void)            { return net_phy_status(); }
uint16_t EthernetClass::phyRegister(uint8_t reg)   { return net_phy_read(reg); }

void EthernetClass::begin(uint8_t *mac, IPAddress ip)
{
    IPAddress gw(ip[0], ip[1], ip[2], 1);
    begin(mac, ip, gw, gw, IPAddress(255, 255, 255, 0));
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns)
{
    IPAddress gw(ip[0], ip[1], ip[2], 1);
    begin(mac, ip, dns, gw, IPAddress(255, 255, 255, 0));
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway)
{
    begin(mac, ip, dns, gateway, IPAddress(255, 255, 255, 0));
}

void EthernetClass::begin(uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet)
{
    uint8_t i[4], g[4], m[4];
    ip_to_bytes(ip, i); ip_to_bytes(gateway, g); ip_to_bytes(subnet, m);
    _dns = dns;
    dbg("[ETH] IP fija: iniciando pila TCP/IP... ");
    dbg(init(mac, i, g, m) ? "OK\r\n" : "ERROR\r\n");
    dbg(waitForLink(3000) ? "[ETH] Enlace OK\r\n" : "[ETH] Aun sin enlace (revise el cable)\r\n");    /* no es obligatorio: la pila funciona en cuanto haya enlace */
}

int EthernetClass::maintain(void)
{
    net_poll();
    return 0;
}

EthernetLinkStatus EthernetClass::linkStatus(void)
{
    if (!net_ready()) return Unknown;
    net_poll();
    return net_link_up() ? LinkON : LinkOFF;
}

IPAddress EthernetClass::localIP(void)    { uint8_t a[4]; net_get_config(a, nullptr, nullptr); return IPAddress(a[0], a[1], a[2], a[3]); }
IPAddress EthernetClass::gatewayIP(void)  { uint8_t a[4]; net_get_config(nullptr, a, nullptr); return IPAddress(a[0], a[1], a[2], a[3]); }
IPAddress EthernetClass::subnetMask(void) { uint8_t a[4]; net_get_config(nullptr, nullptr, a); return IPAddress(a[0], a[1], a[2], a[3]); }
void      EthernetClass::MACAddress(uint8_t *mac) { net_get_mac(mac); }

int EthernetClass::hostByName(const char *host, IPAddress &result, unsigned long timeout)
{
    /* Si ya es una IP en texto, no hace falta DNS */
    if (result.fromString(host)) return 1;
    DNSClient dns;
    dns.begin(_dns);
    return dns.getHostByName(host, result, timeout) == 1 ? 1 : 0;
}
