#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// Common settings used in most of the pico_w examples
#ifndef NO_SYS
#define NO_SYS                      1
#endif

#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0
#endif

#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
#define MEM_LIBC_MALLOC             0
#endif

// ================= CONFIGURAÇÕES DE MEMÓRIA =================
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    32000        // Aumentado para HTML grande
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              48           // Aumentado

// ================= CONFIGURAÇÕES TCP =================
// IMPORTANTE: A ordem dos cálculos importa!
#define TCP_MSS                     1460         // Maximum Segment Size
#define TCP_WND                     (8 * TCP_MSS)   // 11680 bytes
#define TCP_SND_BUF                 (8 * TCP_MSS)   // 11680 bytes

// Calcular TCP_SND_QUEUELEN baseado nos valores acima
// Fórmula: (2 * TCP_SND_BUF + TCP_MSS - 1) / TCP_MSS
// = (2 * 11680 + 1459) / 1460 = 17 segmentos
#define TCP_SND_QUEUELEN            ((2 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// MEMP_NUM_TCP_SEG deve ser >= TCP_SND_QUEUELEN
// TCP_SND_QUEUELEN = 17, então usamos 32 para ter margem
#define MEMP_NUM_TCP_SEG            32

// ================= OUTRAS CONFIGURAÇÕES TCP =================
#define TCP_TMR_INTERVAL            250
#define TCP_MAXRTX                  12
#define TCP_SYNMAXRTX               6

// ================= PROTOCOLOS =================
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1

// ================= INTERFACE DE REDE =================
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// ================= OUTRAS CONFIGURAÇÕES =================
#define LWIP_NETCONN                0
#define LWIP_CHKSUM_ALGORITHM       3
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// ================= ESTATÍSTICAS (desabilitadas) =================
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0

// ================= DEBUG (tudo desabilitado para produção) =================
#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */