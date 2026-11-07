/*
 * ble_core.c - Capa en C sobre la pila Bluetooth LE 4.2 de WCH (CH57xBLE.LIB).
 *
 * Basado en los ejemplos oficiales de WCH (Broadcaster, Observer, Peripheral,
 * Central). Diferencias:
 *   - La tabla de tareas (tasksArr) se arma al iniciar segun el rol, asi el
 *     enlazador solo incluye el codigo del rol que el sketch usa.
 *   - El SysTick sigue siendo el del core (millis); la pila usa el RTC con el
 *     RC interno de 32 kHz calibrado contra el cristal (no hace falta un
 *     cristal de 32.768 kHz).
 *   - Periferico y central intercambian datos con el "Nordic UART Service"
 *     (NUS), que entienden las apps nRF Connect, nRF Toolbox y Serial
 *     Bluetooth Terminal.
 *   - Sin guardar emparejamientos; la pila reserva los ultimos 512 bytes de
 *     la DataFlash (EEPROM 1536..2047), el resto queda para EEPROM.
 */
#include "Arduino.h"
#include "CH57xBLE_LIB.h"
#include "ble_core.h"

#if !defined(CH579_CLOCK_HSE_32MHZ)
#error "BLE necesita el cristal de 32 MHz: Herramientas > Reloj del sistema > Cristal externo 32 MHz (HSE)"
#endif

/* ------------------------------------------------------------ memoria */
#define BLE_MEM_SIZE   (4 * 1024)          /* minimo que pide la pila */
#define RX_SIZE        128                 /* potencias de 2 */
#define TX_SIZE        128
#define BLE_SNV_ADDR   0x3EE00             /* DataFlash 0x3E800 + 1536 */

static __attribute__((aligned(4))) uint32_t s_mem[BLE_MEM_SIZE / 4];

/* La pila recorre esta tabla; se llena en begin segun el rol */
pTaskEventHandlerFn tasksArr[11];
tmosTaskID TASK_CNT;

/* ------------------------------------------------------------- estado */
#define EVT_START   0x0001
#define EVT_TX      0x0002
#define EVT_DISC    0x0004                 /* central: descubrir el servicio */

static tmosTaskID s_app;
static uint8_t  s_role;                    /* GAP_PROFILE_*, 0 = sin iniciar */
static volatile uint8_t s_ready;
static uint8_t  s_in_poll;
static uint8_t  s_addr[6];
static uint8_t  s_txpow = LL_TX_POWEER_0_DBM;
static uint16_t s_adv_int = 160;           /* 100 ms en pasos de 0.625 ms */
static uint8_t  s_scanning, s_scan_active;

static ble_report_cb_t s_report_cb;
static ble_void_cb_t   s_on_conn, s_on_disc;

/* lo que cambia segun el rol */
static void   (*s_start)(void);
static void   (*s_on_msg)(tmos_event_hdr_t *m);
static uint16 (*s_on_evt)(uint16 ev);

/* conexion */
static volatile uint16_t s_conn = INVALID_CONNHANDLE;
static volatile uint8_t  s_link;           /* conectado y listo para datos */
static uint8_t  s_rx[RX_SIZE], s_tx[TX_SIZE];
static volatile uint8_t s_rx_h, s_rx_t, s_tx_h, s_tx_t;

/* ------------------------------------------------------------ comunes */
static uint32 rand_cb(void)  { return SysTick->VAL ^ (micros() << 7) ^ R32_RTC_CNT_32K; }
static void   lsi_cal(void)  { Calibration_LSI(); }

static uint16 hal_task(uint8 id, uint16 ev)
{
    if (ev & SYS_EVENT_MSG) {
        uint8 *m = tmos_msg_receive(id);
        if (m) tmos_msg_deallocate(m);
        return ev ^ SYS_EVENT_MSG;
    }
    return 0;
}

static uint16 app_task(uint8 id, uint16 ev)
{
    if (ev & SYS_EVENT_MSG) {
        uint8 *m = tmos_msg_receive(id);
        if (m) {
            if (s_on_msg) s_on_msg((tmos_event_hdr_t *)m);
            tmos_msg_deallocate(m);
        }
        return ev ^ SYS_EVENT_MSG;
    }
    if (ev & EVT_START) {
        s_start();
        return ev ^ EVT_START;
    }
    return s_on_evt ? s_on_evt(ev) : 0;
}

/* Arranca la pila con las tareas ya cargadas en tasksArr[2..n-2];
 * pone las comunes (0: temporizador, 1: hal, n-1: aplicacion) e inicia la
 * tarea 0 y la base de tiempo (lo que hacen TMOS_Init y Hal_Init en WCH). */
static int stack_init(uint8_t n)
{
    bleConfig_t cfg;
    if (s_role) return -1;                 /* ya iniciada */
    if (tmos_memcmp(VER_LIB, VER_FILE, strlen(VER_FILE)) == FALSE) return -2;

    tasksArr[0] = TMOS_CbTimerProcessEvent;
    tasksArr[1] = hal_task;
    tasksArr[n - 1] = app_task;
    TASK_CNT = n;
    s_app = n - 1;

    tmos_memset(&cfg, 0, sizeof(cfg));
    cfg.MEMAddr        = (u32)s_mem;
    cfg.MEMLen         = sizeof(s_mem);
    /* TMOS_Init siempre prepara un bloque de 512 bytes en flash (con 0
     * borraria la pagina 0 del programa). Va en los ultimos 512 bytes de la
     * DataFlash, como en WCH: EEPROM 1536..2047 queda reservada con BLE. */
    cfg.SNVAddr        = BLE_SNV_ADDR;
    cfg.SNVBlock       = 512;
    cfg.SNVNum         = 1;
    cfg.TxPower        = s_txpow;
    cfg.OscRC32KEnable = 1;                /* 32 kHz del RC interno */
    cfg.TaskCount      = n;
    cfg.TasksArrAddr   = (u32)tasksArr;
    if (BLE_LibInit(&cfg)) return -3;
    TMOS_RandRegister(rand_cb);
    TMOS_Init(0);

    /* base de tiempo de la pila: RTC con el RC de 32 kHz calibrado */
    Calibration_LSI();
    TMOS_Rc32KRegister(lsi_cal);
    RTC_InitTime(2020, 1, 1, 0, 0, 0);
    TMOS_TimerInit();
    return 0;
}

static void set_adv_params(void)
{
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, s_adv_int);
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, s_adv_int);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, s_adv_int);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, s_adv_int);
    GAP_SetParamValue(TGAP_CONN_ADV_INT_MIN, s_adv_int);
    GAP_SetParamValue(TGAP_CONN_ADV_INT_MAX, s_adv_int);
}

/* Espera a que la pila termine de arrancar */
static int wait_ready(void)
{
    uint32_t t0 = millis();
    tmos_set_event(s_app, EVT_START);
    while (!s_ready) {
        ble_poll();
        if (millis() - t0 > 2000) return -4;
    }
    return 0;
}

void ble_poll(void)
{
    if (!s_role || s_in_poll) return;
    s_in_poll = 1;
    TMOS_SystemProcess();
    s_in_poll = 0;
}

uint8_t ble_role(void)               { return s_role; }
const uint8_t *ble_address(void)     { return s_addr; }

void ble_set_tx_power(int8_t dbm)
{
    static const int8_t  d[] = { -20, -14, -8, -3, 0, 1, 2, 3 };
    static const uint8_t v[] = { LL_TX_POWEER_MINUS_20_DBM, LL_TX_POWEER_MINUS_14_DBM,
                                 LL_TX_POWEER_MINUS_8_DBM, LL_TX_POWEER_MINUS_3_DBM,
                                 LL_TX_POWEER_0_DBM, LL_TX_POWEER_1_DBM,
                                 LL_TX_POWEER_2_DBM, LL_TX_POWEER_3_DBM };
    uint8_t i = 0;
    while (i < 7 && dbm >= d[i + 1]) i++;
    s_txpow = v[i];
}

void ble_set_adv_interval(uint16_t ms)
{
    uint32_t n = (uint32_t)ms * 8 / 5;     /* pasos de 0.625 ms */
    if (n < 32) n = 32;                    /* 20 ms */
    if (n > 16384) n = 16384;              /* 10.24 s */
    s_adv_int = n;
    if (s_role) set_adv_params();          /* vale desde el proximo arranque del anuncio */
}

/* --------------------------------------------------------- anuncios */
static uint8_t s_adv_n, s_rsp_n;
static uint8_t s_adv[B_MAX_ADV_LEN], s_rsp[B_MAX_ADV_LEN];

int ble_set_adv_data(const uint8_t *d, uint8_t n)
{
    if (n > B_MAX_ADV_LEN) return -1;
    memcpy(s_adv, d, n);
    s_adv_n = n;
    if (s_role & (GAP_PROFILE_BROADCASTER | GAP_PROFILE_PERIPHERAL))
        return GAPRole_SetParameter(GAPROLE_ADVERT_DATA, n, s_adv);
    return 0;
}

int ble_set_scan_rsp(const uint8_t *d, uint8_t n)
{
    if (n > B_MAX_ADV_LEN) return -1;
    memcpy(s_rsp, d, n);
    s_rsp_n = n;
    if (s_role & (GAP_PROFILE_BROADCASTER | GAP_PROFILE_PERIPHERAL))
        return GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, n, s_rsp);
    return 0;
}

int ble_advertise(uint8_t on)
{
    if (!(s_role & (GAP_PROFILE_BROADCASTER | GAP_PROFILE_PERIPHERAL))) return -1;
    on = on ? TRUE : FALSE;
    return GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, 1, &on);
}

/* estado de difusor y periferico */
static void role_state_cb(gaprole_States_t st)
{
    switch (st) {
    case GAPROLE_STARTED:
        GAPRole_GetParameter(GAPROLE_BD_ADDR, s_addr);
        s_ready = 1;
        break;
    case GAPROLE_ADVERTISING:
        s_ready = 1;
        break;
    case GAPROLE_CONNECTED:
        if (s_role == GAP_PROFILE_PERIPHERAL) {
            uint16 h;
            GAPRole_GetParameter(GAPROLE_CONNHANDLE, &h);
            s_conn = h;
            s_link = 1;
            s_rx_h = s_rx_t = s_tx_h = s_tx_t = 0;
            if (s_on_conn) s_on_conn();
        }
        break;
    case GAPROLE_WAITING:
    case GAPROLE_WAITING_AFTER_TIMEOUT:
        if (s_link) {
            s_link = 0;
            s_conn = INVALID_CONNHANDLE;
            if (s_on_disc) s_on_disc();
        }
        break;
    default:
        break;
    }
}

static gapRolesCBs_t s_role_cbs = { role_state_cb, NULL };

/* ------------------------------------------------------------ difusor */
static void bc_start(void)
{
    GAPRole_BroadcasterStartDevice(&s_role_cbs);
}

int ble_begin_broadcaster(void)
{
    uint8_t i = 2, t, en = TRUE;
    /* las mismas tareas que el ejemplo Broadcaster de WCH */
    tasksArr[i++] = LL_ProcessEvent;
    tasksArr[i++] = L2CAP_ProcessEvent;
    tasksArr[i++] = GAP_ProcessEvent;
    tasksArr[i++] = GATT_ProcessEvent;
    tasksArr[i++] = SM_ProcessEvent;
    tasksArr[i++] = GAPBondMgr_ProcessEvent;
    tasksArr[i++] = GATTServApp_ProcessEvent;
    tasksArr[i++] = GAPRole_BroadcasterProcessEvent;
    if (stack_init(i + 1)) return -1;
    LL_Init(2); L2CAP_Init(3); GAP_Init(4); GATT_Init(5); SM_Init(6);
    GAPBondMgr_Init(7); GATTServApp_Init(8); GAPRole_BroadcasterInit(9);
    s_role = GAP_PROFILE_BROADCASTER;

    /* sin respuesta de escaneo: anuncio no conectable; con ella: escaneable */
    t = s_rsp_n ? GAP_ADTYPE_ADV_SCAN_IND : GAP_ADTYPE_ADV_NONCONN_IND;
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, 1, &t);
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, 1, &en);
    if (s_adv_n) GAPRole_SetParameter(GAPROLE_ADVERT_DATA, s_adv_n, s_adv);
    if (s_rsp_n) GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, s_rsp_n, s_rsp);
    set_adv_params();
    s_start = bc_start;
    return wait_ready();
}

/* --------------------------------------------- escaneo (observador/central) */
static void report(const gapDeviceInfoEvent_t *e)
{
    ble_report_t r;
    if (!s_report_cb) return;
    memcpy(r.addr, e->addr, 6);
    r.addrType = e->addrType;
    r.evtType  = e->eventType;
    r.rssi     = e->rssi;
    r.len      = e->dataLen;
    r.data     = e->pEvtData;
    s_report_cb(&r);
}

void ble_set_report_cb(ble_report_cb_t cb) { s_report_cb = cb; }

static void scan_params(void)
{
    uint8 n = 0;                           /* la lista interna no se usa */
    GAPRole_SetParameter(GAPROLE_MAX_SCAN_RES, 1, &n);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN, 4800);   /* tandas de 3 s */
    GAP_SetParamValue(TGAP_LIM_DISC_SCAN, 4800);
}

/* ---------------------------------------------------------- observador */
static void obs_event(gapObserverRoleEvent_t *e)
{
    switch (e->gap.opcode) {
    case GAP_DEVICE_INIT_DONE_EVENT:
        memcpy(s_addr, e->initDone.devAddr, 6);
        s_ready = 1;
        break;
    case GAP_DEVICE_INFO_EVENT:
        report(&e->deviceInfo);
        break;
    case GAP_DEVICE_DISCOVERY_EVENT:       /* fin de una tanda: sigue */
        if (s_scanning) GAPRole_ObserverStartDiscovery(DEVDISC_MODE_ALL, s_scan_active, FALSE);
        break;
    }
}

static const gapObserverRoleCB_t s_obs_cbs = { NULL, obs_event };

static void obs_start(void)
{
    GAPRole_ObserverStartDevice((gapObserverRoleCB_t *)&s_obs_cbs);
}

int ble_begin_observer(void)
{
    uint8_t i = 2;
    /* las mismas tareas que el ejemplo Observer de WCH */
    tasksArr[i++] = LL_ProcessEvent;
    tasksArr[i++] = GAP_ProcessEvent;
    tasksArr[i++] = GAPRole_ObserverProcessEvent;
    if (stack_init(i + 1)) return -1;
    LL_Init(2); GAP_Init(3); GAPRole_ObserverInit(4);
    s_role = GAP_PROFILE_OBSERVER;
    scan_params();
    s_start = obs_start;
    return wait_ready();
}

/* --------------------------------------- servicio UART (Nordic UART Service) */
#define NUS_UUID(n) { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
                      0x93, 0xF3, 0xA3, 0xB5, (n), 0x00, 0x40, 0x6E }
static const uint8 s_nus_uuid[ATT_UUID_SIZE] = NUS_UUID(1);
static const uint8 s_rx_uuid[ATT_UUID_SIZE]  = NUS_UUID(2);   /* el movil escribe */
static const uint8 s_tx_uuid[ATT_UUID_SIZE]  = NUS_UUID(3);   /* la placa notifica */

static int rx_push(const uint8_t *d, uint16_t n)
{
    while (n--) {
        uint8_t h = (s_rx_h + 1) & (RX_SIZE - 1);
        if (h == s_rx_t) return -1;        /* lleno: se pierde el resto */
        s_rx[s_rx_h] = *d++;
        s_rx_h = h;
    }
    return 0;
}

/* ---------------------------------------------------------- periferico */
static const gattAttrType_t s_nus_svc = { ATT_UUID_SIZE, s_nus_uuid };
static uint8 s_rx_props = GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8 s_tx_props = GATT_PROP_NOTIFY;
static uint8 s_dummy;
static gattCharCfg_t s_tx_ccc[GATT_MAX_NUM_CONN];

static gattAttribute_t s_nus_tbl[] = {
    { { ATT_BT_UUID_SIZE, primaryServiceUUID }, GATT_PERMIT_READ, 0, (uint8 *)&s_nus_svc },
    { { ATT_BT_UUID_SIZE, characterUUID },      GATT_PERMIT_READ, 0, &s_rx_props },
    { { ATT_UUID_SIZE,    s_rx_uuid },          GATT_PERMIT_WRITE, 0, &s_dummy },
    { { ATT_BT_UUID_SIZE, characterUUID },      GATT_PERMIT_READ, 0, &s_tx_props },
    { { ATT_UUID_SIZE,    s_tx_uuid },          0, 0, &s_dummy },
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },  GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *)s_tx_ccc },
};
#define NUS_TX_ATTR  (&s_nus_tbl[4])
#define NUS_CCC_ATTR (&s_nus_tbl[5])

static uint8 nus_read(uint16 conn, gattAttribute_t *a, uint8 *v, uint16 *len,
                      uint16 off, uint16 max, uint8 method)
{
    (void)conn; (void)a; (void)v; (void)off; (void)max; (void)method;
    *len = 0;
    return ATT_ERR_READ_NOT_PERMITTED;
}

static uint8 nus_write(uint16 conn, gattAttribute_t *a, uint8 *v, uint16 len,
                       uint16 off, uint8 method)
{
    (void)method;
    if (a == NUS_CCC_ATTR) {
        uint8 r = GATTServApp_ProcessCCCWriteReq(conn, a, v, len, off, GATT_CLIENT_CFG_NOTIFY);
        if (r == SUCCESS) tmos_set_event(s_app, EVT_TX);
        return r;
    }
    if (a == &s_nus_tbl[2]) {
        if (off) return ATT_ERR_ATTR_NOT_LONG;
        rx_push(v, len);
        return SUCCESS;
    }
    return ATT_ERR_ATTR_NOT_FOUND;
}

static gattServiceCBs_t s_nus_cbs = { nus_read, nus_write, NULL };

static void nus_link_cb(uint16 conn, uint8 change)
{
    if (conn != LOOPBACK_CONNHANDLE &&
        (change == LINKDB_STATUS_UPDATE_REMOVED ||
         (change == LINKDB_STATUS_UPDATE_STATEFLAGS && !linkDB_Up(conn))))
        GATTServApp_InitCharCfg(conn, s_tx_ccc);
}

static int tx_len(void) { return (s_tx_h - s_tx_t) & (TX_SIZE - 1); }

/* Cuantos bytes caben en un paquete */
static uint16_t chunk(void)
{
    uint16_t m = ATT_GetMTU(s_conn);
    m = (m > 3) ? m - 3 : 20;
    if (m > (uint16_t)tx_len()) m = tx_len();
    return m;
}

static uint16 per_evt(uint16 ev)
{
    if (ev & EVT_TX) {
        uint8_t k;
        /* sin notificaciones activadas se espera: el CCC vuelve a pedir EVT_TX */
        if (!(GATTServApp_ReadCharCfg(s_conn, s_tx_ccc) & GATT_CLIENT_CFG_NOTIFY))
            return ev ^ EVT_TX;
        for (k = 0; k < 4 && s_link && tx_len(); k++) {
            attHandleValueNoti_t no;
            uint16_t n = chunk();
            no.pValue = GATT_bm_alloc(s_conn, ATT_HANDLE_VALUE_NOTI, n, NULL, 0);
            if (!no.pValue) break;
            no.handle = NUS_TX_ATTR->handle;
            no.len = n;
            {   /* copia sin sacar del buffer: si falla se reintenta */
                uint8_t t = s_tx_t, j;
                for (j = 0; j < n; j++) { no.pValue[j] = s_tx[t]; t = (t + 1) & (TX_SIZE - 1); }
            }
            if (GATT_Notification(s_conn, &no, FALSE) != SUCCESS) {
                GATT_bm_free((gattMsg_t *)&no, ATT_HANDLE_VALUE_NOTI);
                break;
            }
            s_tx_t = (s_tx_t + n) & (TX_SIZE - 1);
        }
        if (s_link && tx_len()) tmos_start_task(s_app, EVT_TX, MS1_TO_SYSTEM_TIME(5));
        return ev ^ EVT_TX;
    }
    return 0;
}

static gapBondCBs_t s_bond_cbs = { NULL, NULL };

static void per_start(void)
{
    GAPRole_PeripheralStartDevice(&s_role_cbs);
    GAPBondMgr_Register(&s_bond_cbs);
}

/* Emparejamiento "Just Works" solo si el otro lo pide; sin guardar claves */
static void bond_params(void)
{
    uint8 mode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ, no = FALSE;
    uint8 io = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE, 1, &mode);
    GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION, 1, &no);
    GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES, 1, &io);
    GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED, 1, &no);
}

static void set_name(const char *name)
{
    uint8 n[GAP_DEVICE_NAME_LEN];
    tmos_memset(n, 0, sizeof(n));
    strncpy((char *)n, name, GAP_DEVICE_NAME_LEN);
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, n);
}

int ble_begin_peripheral(const char *name)
{
    uint8_t i = 2, en = TRUE;
    /* las mismas tareas que el ejemplo Peripheral de WCH */
    tasksArr[i++] = LL_ProcessEvent;
    tasksArr[i++] = L2CAP_ProcessEvent;
    tasksArr[i++] = GAP_ProcessEvent;
    tasksArr[i++] = GATT_ProcessEvent;
    tasksArr[i++] = SM_ProcessEvent;
    tasksArr[i++] = GAPBondMgr_ProcessEvent;
    tasksArr[i++] = GATTServApp_ProcessEvent;
    tasksArr[i++] = GAPRole_PeripheralProcessEvent;
    if (stack_init(i + 1)) return -1;
    LL_Init(2); L2CAP_Init(3); GAP_Init(4); GATT_Init(5); SM_Init(6);
    GAPBondMgr_Init(7); GATTServApp_Init(8); GAPRole_PeripheralInit(9);
    s_role = GAP_PROFILE_PERIPHERAL;

    /* anuncio por omision: flags + UUID del servicio UART; nombre en la respuesta */
    if (!s_adv_n) {
        uint8_t *p = s_adv;
        *p++ = 2; *p++ = GAP_ADTYPE_FLAGS;
        *p++ = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
        *p++ = 17; *p++ = GAP_ADTYPE_128BIT_COMPLETE;
        memcpy(p, s_nus_uuid, 16);
        s_adv_n = 21;
    }
    if (!s_rsp_n) {
        uint8_t n = strlen(name);
        if (n > B_MAX_ADV_LEN - 2) n = B_MAX_ADV_LEN - 2;
        s_rsp[0] = n + 1;
        s_rsp[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
        memcpy(s_rsp + 2, name, n);
        s_rsp_n = n + 2;
    }
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, 1, &en);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, s_adv_n, s_adv);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, s_rsp_n, s_rsp);
    set_adv_params();
    set_name(name);
    bond_params();

    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, s_tx_ccc);
    linkDB_Register(nus_link_cb);
    GATTServApp_RegisterService(s_nus_tbl, sizeof(s_nus_tbl) / sizeof(s_nus_tbl[0]),
                                GATT_MAX_ENCRYPT_KEY_SIZE, &s_nus_cbs);
    s_start = per_start;
    s_on_evt = per_evt;
    return wait_ready();
}

/* -------------------------------------------------------------- central */
static uint16_t s_svc_s, s_svc_e, s_rx_hdl, s_tx_hdl;
/* descubrimiento: servicio UART -> caracteristicas -> CCC */
enum { DISC_NO, DISC_SVC, DISC_CHAR, DISC_CCC };
static uint8_t  s_disc;
static uint8_t  s_pend_addr[6], s_pend_type, s_pending, s_cen_busy;

static void cen_scan(void)
{
    GAPRole_CentralStartDiscovery(DEVDISC_MODE_ALL, s_scan_active, FALSE);
}

static void cen_link_down(void)
{
    uint8_t was = s_link;
    s_conn = INVALID_CONNHANDLE;
    s_link = 0;
    s_cen_busy = 0;
    s_rx_hdl = s_tx_hdl = 0;
    s_disc = DISC_NO;
    if (was && s_on_disc) s_on_disc();
}

static void cen_event(gapCentralRoleEvent_t *e)
{
    switch (e->gap.opcode) {
    case GAP_DEVICE_INIT_DONE_EVENT:
        memcpy(s_addr, e->initDone.devAddr, 6);
        s_ready = 1;
        break;
    case GAP_DEVICE_INFO_EVENT:
        report(&e->deviceInfo);
        break;
    case GAP_DEVICE_DISCOVERY_EVENT:
        if (s_pending) {                   /* el escaneo termino: ahora conecta */
            s_pending = 0;
            s_cen_busy = 1;
            GAPRole_CentralEstablishLink(FALSE, FALSE, s_pend_type, s_pend_addr);
        } else if (s_scanning) {
            cen_scan();
        }
        break;
    case GAP_LINK_ESTABLISHED_EVENT:
        if (e->gap.hdr.status == SUCCESS) {
            s_conn = e->linkCmpl.connectionHandle;
            s_rx_h = s_rx_t = s_tx_h = s_tx_t = 0;
            tmos_start_task(s_app, EVT_DISC, MS1_TO_SYSTEM_TIME(500));
        } else {
            cen_link_down();
        }
        break;
    case GAP_LINK_TERMINATED_EVENT:
        cen_link_down();
        break;
    }
}

static const gapCentralRoleCB_t s_cen_cbs = { NULL, cen_event, NULL };

static void cen_ready(void)
{
    s_disc = DISC_NO;
    if (s_link) return;
    s_link = 1;
    s_cen_busy = 0;
    if (s_on_conn) s_on_conn();
    tmos_set_event(s_app, EVT_TX);
}

/* Escribe en el CCC del TX remoto para recibir sus notificaciones */
static void cen_subscribe(void)
{
    attWriteReq_t w;
    if (!s_rx_hdl || !s_tx_hdl) {          /* no es un UART BLE */
        s_disc = DISC_NO;
        ble_disconnect();
        return;
    }
    s_disc = DISC_CCC;
    w.pValue = GATT_bm_alloc(s_conn, ATT_WRITE_REQ, 2, NULL, 0);
    if (!w.pValue) { cen_ready(); return; }
    w.handle = s_tx_hdl + 1;               /* el CCC sigue al valor en el NUS */
    w.len = 2;
    w.pValue[0] = LO_UINT16(GATT_CLIENT_CFG_NOTIFY);
    w.pValue[1] = HI_UINT16(GATT_CLIENT_CFG_NOTIFY);
    w.sig = FALSE;
    w.cmd = FALSE;
    if (GATT_WriteCharValue(s_conn, &w, s_app) != SUCCESS) {
        GATT_bm_free((gattMsg_t *)&w, ATT_WRITE_REQ);
        cen_ready();
    }
}

static void cen_chars(void)
{
    if (!s_svc_s) {                        /* no tiene el servicio UART */
        s_disc = DISC_NO;
        ble_disconnect();
        return;
    }
    s_disc = DISC_CHAR;
    GATT_DiscAllChars(s_conn, s_svc_s, s_svc_e, s_app);
}

static void cen_gatt(gattMsgEvent_t *m)
{
    uint8_t fin_proc;
    if (m->connHandle != s_conn) goto fin;
    if (m->method == ATT_HANDLE_VALUE_NOTI) {
        if (m->msg.handleValueNoti.handle == s_tx_hdl)
            rx_push(m->msg.handleValueNoti.pValue, m->msg.handleValueNoti.len);
        goto fin;
    }
    fin_proc = (m->hdr.status == bleProcedureComplete) || (m->method == ATT_ERROR_RSP);
    switch (s_disc) {
    case DISC_SVC:
        if (m->method == ATT_FIND_BY_TYPE_VALUE_RSP && m->msg.findByTypeValueRsp.numInfo > 0) {
            s_svc_s = ATT_ATTR_HANDLE(m->msg.findByTypeValueRsp.pHandlesInfo, 0);
            s_svc_e = ATT_GRP_END_HANDLE(m->msg.findByTypeValueRsp.pHandlesInfo, 0);
        }
        if (fin_proc) cen_chars();
        break;
    case DISC_CHAR:
        if (m->method == ATT_READ_BY_TYPE_RSP) {
            uint8_t k;
            attReadByTypeRsp_t *r = &m->msg.readByTypeRsp;
            for (k = 0; k < r->numPairs && r->len == 21; k++) {
                uint8 *p = r->pDataList + k * 21;     /* decl(2) prop(1) valor(2) uuid(16) */
                uint16_t h = BUILD_UINT16(p[3], p[4]);
                if (!memcmp(p + 5, s_rx_uuid, 16)) s_rx_hdl = h;
                if (!memcmp(p + 5, s_tx_uuid, 16)) s_tx_hdl = h;
            }
        }
        if (fin_proc) cen_subscribe();
        break;
    case DISC_CCC:                         /* suscrito (o el otro no lo pide) */
        if (m->method == ATT_WRITE_RSP || m->method == ATT_ERROR_RSP) cen_ready();
        break;
    }
fin:
    GATT_bm_free(&m->msg, m->method);
}

static void cen_msg(tmos_event_hdr_t *m)
{
    if (m->event == GATT_MSG_EVENT) cen_gatt((gattMsgEvent_t *)m);
}

static uint16 cen_evt(uint16 ev)
{
    if (ev & EVT_DISC) {
        s_svc_s = s_svc_e = s_rx_hdl = s_tx_hdl = 0;
        s_disc = DISC_SVC;
        GATT_DiscPrimaryServiceByUUID(s_conn, (uint8 *)s_nus_uuid, ATT_UUID_SIZE, s_app);
        return ev ^ EVT_DISC;
    }
    if (ev & EVT_TX) {
        uint8_t k;
        for (k = 0; k < 4 && s_link && tx_len(); k++) {
            attWriteReq_t w;
            uint16_t n = chunk();
            w.pValue = GATT_bm_alloc(s_conn, ATT_WRITE_CMD, n, NULL, 0);
            if (!w.pValue) break;
            w.handle = s_rx_hdl;
            w.len = n;
            w.sig = FALSE;
            w.cmd = TRUE;
            {
                uint8_t t = s_tx_t, j;
                for (j = 0; j < n; j++) { w.pValue[j] = s_tx[t]; t = (t + 1) & (TX_SIZE - 1); }
            }
            if (GATT_WriteNoRsp(s_conn, &w) != SUCCESS) {
                GATT_bm_free((gattMsg_t *)&w, ATT_WRITE_CMD);
                break;
            }
            s_tx_t = (s_tx_t + n) & (TX_SIZE - 1);
        }
        if (s_link && tx_len()) tmos_start_task(s_app, EVT_TX, MS1_TO_SYSTEM_TIME(5));
        return ev ^ EVT_TX;
    }
    return 0;
}

static void cen_start(void)
{
    GAPRole_CentralStartDevice((gapCentralRoleCB_t *)&s_cen_cbs);
    GAPBondMgr_Register(&s_bond_cbs);
}

int ble_begin_central(void)
{
    uint8_t i = 2;
    /* las mismas tareas que el ejemplo Central de WCH */
    tasksArr[i++] = LL_ProcessEvent;
    tasksArr[i++] = L2CAP_ProcessEvent;
    tasksArr[i++] = GAP_ProcessEvent;
    tasksArr[i++] = GATT_ProcessEvent;
    tasksArr[i++] = SM_ProcessEvent;
    tasksArr[i++] = GAPBondMgr_ProcessEvent;
    tasksArr[i++] = GATTServApp_ProcessEvent;
    tasksArr[i++] = GAPRole_CentralProcessEvent;
    if (stack_init(i + 1)) return -1;
    LL_Init(2); L2CAP_Init(3); GAP_Init(4); GATT_Init(5); SM_Init(6);
    GAPBondMgr_Init(7); GATTServApp_Init(8); GAPRole_CentralInit(9);
    s_role = GAP_PROFILE_CENTRAL;

    scan_params();
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, 8);       /* 10 ms */
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, 8);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 100);   /* 1 s */
    set_name("CH579 central");
    bond_params();
    GATT_InitClient();
    GATT_RegisterForInd(s_app);
    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);
    s_start = cen_start;
    s_on_msg = cen_msg;
    s_on_evt = cen_evt;
    return wait_ready();
}

int ble_scan(uint8_t active)
{
    s_scan_active = active ? TRUE : FALSE;
    if (s_scanning) return 0;
    s_scanning = 1;
    if (s_role == GAP_PROFILE_OBSERVER)
        return GAPRole_ObserverStartDiscovery(DEVDISC_MODE_ALL, s_scan_active, FALSE);
    if (s_role == GAP_PROFILE_CENTRAL) {
        if (s_conn != INVALID_CONNHANDLE || s_cen_busy) return -1;
        cen_scan();
        return 0;
    }
    s_scanning = 0;
    return -1;
}

void ble_stop_scan(void)
{
    if (!s_scanning) return;
    s_scanning = 0;
    if (s_role == GAP_PROFILE_OBSERVER) GAPRole_ObserverCancelDiscovery();
    if (s_role == GAP_PROFILE_CENTRAL)  GAPRole_CentralCancelDiscovery();
}

int ble_connect(const uint8_t addr[6], uint8_t addrType)
{
    if (s_role != GAP_PROFILE_CENTRAL || s_conn != INVALID_CONNHANDLE || s_cen_busy || s_pending)
        return -1;
    memcpy(s_pend_addr, addr, 6);
    s_pend_type = addrType;
    if (s_scanning) {                      /* hay que parar el escaneo antes */
        s_scanning = 0;
        s_pending = 1;
        GAPRole_CentralCancelDiscovery();
        return 0;
    }
    s_cen_busy = 1;
    return GAPRole_CentralEstablishLink(FALSE, FALSE, addrType, s_pend_addr);
}

void ble_disconnect(void)
{
    if (s_role == GAP_PROFILE_PERIPHERAL && s_conn != INVALID_CONNHANDLE)
        GAPRole_PeripheralTerminateLink();
    if (s_role == GAP_PROFILE_CENTRAL) {
        if (s_conn != INVALID_CONNHANDLE) GAPRole_CentralTerminateLink(s_conn);
        else if (s_cen_busy) GAPRole_CentralTerminateLink(GAP_CONNHANDLE_INIT);   /* cancela el intento */
    }
}

/* ------------------------------------------------- datos (como Serial) */
void ble_set_conn_cbs(ble_void_cb_t c, ble_void_cb_t d) { s_on_conn = c; s_on_disc = d; }
uint8_t ble_connected(void) { return s_link; }

int ble_available(void) { return (s_rx_h - s_rx_t) & (RX_SIZE - 1); }

int ble_peek(void)
{
    return s_rx_h == s_rx_t ? -1 : s_rx[s_rx_t];
}

int ble_read(void)
{
    int c;
    if (s_rx_h == s_rx_t) return -1;
    c = s_rx[s_rx_t];
    s_rx_t = (s_rx_t + 1) & (RX_SIZE - 1);
    return c;
}

int ble_tx_free(void) { return TX_SIZE - 1 - tx_len(); }

/* Copia al buffer de envio. Si se llena espera (hasta 1 s) a que la pila
 * envie; sin conexion descarta, como un puerto serie sin nadie escuchando. */
uint16_t ble_write(const uint8_t *d, uint16_t n)
{
    uint16_t k = 0;
    uint32_t t0 = millis();
    while (k < n && s_link) {
        uint8_t h = (s_tx_h + 1) & (TX_SIZE - 1);
        if (h == s_tx_t) {                 /* lleno */
            if (s_in_poll || millis() - t0 > 1000) break;
            tmos_set_event(s_app, EVT_TX);
            ble_poll();
            continue;
        }
        s_tx[s_tx_h] = d[k++];
        s_tx_h = h;
    }
    if (k) tmos_set_event(s_app, EVT_TX);
    return k;
}

void ble_flush(void)
{
    uint32_t t0 = millis();
    while (s_link && tx_len() && !s_in_poll && millis() - t0 < 1000) ble_poll();
}

/* -------------------------------------------------- interrupciones */
/* La pila usa el RTC como base de tiempo; esta solo limpia los avisos */
void RTC_IRQHandler(void)
{
    R8_RTC_FLAG_CTRL = RB_RTC_TMR_CLR | RB_RTC_TRIG_CLR;
}

/* Asegura que el enlazador tome los manejadores de radio de la libreria
 * y no los vacios (weak) del arranque */
extern void BB_IRQHandler(void);
extern void LLE_IRQHandler(void);
void (*const ble_irq_keep[])(void) __attribute__((used)) = { BB_IRQHandler, LLE_IRQHandler };
