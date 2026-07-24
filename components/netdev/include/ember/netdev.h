/*
 * Copyright (c) 2022-2023, OpenEmber Team
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-15     luhuadong    the first version
 * 2026-07-25     openember    migrate to emberlite
 */

#ifndef EMBER_NETDEV_H
#define EMBER_NETDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EMBER_EOK
#define EMBER_EOK    0
#endif
#ifndef EMBER_ERROR
#define EMBER_ERROR  1
#endif

#define NAME_SIZE       8
#define MAC_SIZE        18
#define IP_SIZE         16
#define MASK_SIZE       24
#define GATEWAY_SIZE    24

struct netdev_attr
{
    char ipaddr[IP_SIZE];
    char netmask[IP_SIZE];
    char gateway[IP_SIZE];
    char dns[IP_SIZE];
};
typedef struct netdev_attr netdev_attr_t;

int get_local_mac(const char *ifname, char *mac);
int get_local_ip(const char *ifname, char *ip);
int get_local_netmask(const char *ifname, char* netmask_addr);
int get_local_gateway(const char *ifname, char* gateway);
int get_local_dns(const char *ifname, char* dns_addr);

int set_local_mac(const char *ifname, const char *mac);
int set_local_ip(const char *ifname, const char *ip);
int set_local_netmask(const char *ifname, const char *netmask_addr);
int set_local_gateway(const char *ifname, const char *gateway);
int set_local_dns(const char *ifname, const char* dns_addr);

int get_ip_attr(const char *ifname, netdev_attr_t *attr);
int set_ip_attr(const char *ifname, const netdev_attr_t *attr);

int get_local_ip_by_socket(const int sockfd, char *ip, int *port);

typedef enum {
    LINK_UP = 0,
    LINK_DOWN
} link_status_t;

int set_netdev_status(const char *ifname, const link_status_t status);
link_status_t get_netdev_status(const char *ifname);


#ifdef __cplusplus
}
#endif

#endif /* EMBER_NETDEV_H */
