#include "Ethernet.h"
#include "EthernetDns.h"

#define DNS_PORT  53

/* Construye la consulta tipo A. Devuelve su longitud o 0 si el nombre es invalido. */
static int build_query(uint8_t *q, int max, const char *host, uint16_t id)
{
    int n = 0;
    q[n++] = (uint8_t)(id >> 8); q[n++] = (uint8_t)id;
    q[n++] = 0x01; q[n++] = 0x00;              /* consulta estandar, recursion deseada */
    q[n++] = 0; q[n++] = 1;                     /* 1 pregunta */
    q[n++] = 0; q[n++] = 0; q[n++] = 0; q[n++] = 0; q[n++] = 0; q[n++] = 0;

    const char *p = host;
    while (*p) {
        const char *dot = strchr(p, '.');
        int len = dot ? (int)(dot - p) : (int)strlen(p);
        if (len == 0 || len > 63 || n + len + 6 > max) return 0;
        q[n++] = (uint8_t)len;
        memcpy(q + n, p, len);
        n += len;
        p += len;
        if (*p == '.') p++;
    }
    q[n++] = 0;
    q[n++] = 0; q[n++] = 1;                     /* tipo A */
    q[n++] = 0; q[n++] = 1;                     /* clase IN */
    return n;
}

/* Salta un nombre (etiquetas o puntero comprimido) */
static int skip_name(const uint8_t *r, int len, int i)
{
    while (i < len) {
        uint8_t l = r[i];
        if (l == 0) return i + 1;
        if ((l & 0xC0) == 0xC0) return i + 2;
        i += l + 1;
    }
    return -1;
}

static int parse_reply(const uint8_t *r, int len, uint16_t id, IPAddress &out)
{
    if (len < 12) return 0;
    if (((r[0] << 8) | r[1]) != id) return 0;
    if (!(r[2] & 0x80) || (r[3] & 0x0F) != 0) return 0;   /* no es respuesta o hubo error */
    int qd = (r[4] << 8) | r[5];
    int an = (r[6] << 8) | r[7];
    int i = 12;
    while (qd-- > 0) {
        i = skip_name(r, len, i);
        if (i < 0) return 0;
        i += 4;
    }
    while (an-- > 0 && i < len) {
        i = skip_name(r, len, i);
        if (i < 0 || i + 10 > len) return 0;
        uint16_t type  = (uint16_t)((r[i] << 8) | r[i + 1]);
        uint16_t klass = (uint16_t)((r[i + 2] << 8) | r[i + 3]);
        uint16_t rdlen = (uint16_t)((r[i + 8] << 8) | r[i + 9]);
        i += 10;
        if (i + rdlen > len) return 0;
        if (type == 1 && klass == 1 && rdlen == 4) {
            out = IPAddress(r[i], r[i + 1], r[i + 2], r[i + 3]);
            return 1;
        }
        i += rdlen;                              /* CNAME u otros: siguiente respuesta */
    }
    return 0;
}

int DNSClient::getHostByName(const char *host, IPAddress &result, unsigned long timeout)
{
    if (!host || !*host) return -1;
    if (result.fromString(host)) return 1;
    if (_server == IPAddress(0, 0, 0, 0)) return -1;

    uint8_t buf[256];
    uint16_t id = (uint16_t)(micros() ^ (millis() << 4));
    int qlen = build_query(buf, sizeof(buf), host, id);
    if (qlen == 0) return -1;

    EthernetUDP udp;
    if (!udp.begin(0)) return 0;

    int ret = 0;
    for (int attempt = 0; attempt < 3 && ret != 1; attempt++) {
        udp.beginPacket(_server, DNS_PORT);
        udp.write(buf, qlen);
        udp.endPacket();
        unsigned long t0 = millis();
        while (millis() - t0 < timeout / 3) {
            int n = udp.parsePacket();
            if (n > 0) {
                if (n > (int)sizeof(buf)) n = sizeof(buf);
                n = udp.read(buf + 0, n);
                if (udp.remotePort() == DNS_PORT && parse_reply(buf, n, id, result)) { ret = 1; break; }
                build_query(buf, sizeof(buf), host, id);   /* restaura la consulta */
            }
        }
    }
    udp.stop();
    return ret;
}
