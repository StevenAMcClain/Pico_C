#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// No TCP/IP stack needed for BLE-only
#define NO_SYS                      1
#define LWIP_RAW                    1
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0

// Memory configuration
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4096

// Disable features not used by BLE
#define LWIP_DHCP                   0
#define LWIP_DNS                    0
#define LWIP_ICMP                   0
#define LWIP_IPV4                   1
#define LWIP_IPV6                   0

// Needed by BTstack integration
#define MEMP_NUM_SYS_TIMEOUT        10

#endif
