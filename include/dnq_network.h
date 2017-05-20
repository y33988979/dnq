#ifndef _DNQ_NETWORK_H_
#define _DNQ_NETWORK_H_

#include "common.h"


#define ETH_NAME  "eth0"

typedef enum _net_status
{
    LINK_OFF,
    LINK_ON,
    IP_REQUEST,
    IP_BOUND,
    IP_LOST,
}net_status_e;

typedef struct _host_net_info
{
    U8  if_name[6];
    U8  mac[6];
    U32 ipaddr;
    U32 mask;
    U32 gateway;
    U32 broadcast;
    U32 route;
    U32 dns;
    U32 dns_ex;
    U32 server_ip;
    U32 port;
}host_net_info_t;

typedef void (*netlink_callback)(net_status_e status);
void netlink_callback_enable(netlink_callback callback);

S32 dnq_net_ifup(U8 *if_name);
S32 dnq_net_ifdown(U8 *if_name);
U32 dnq_net_get_dns();
S32 dnq_net_get_dns2(U32 *dnsaddrs);
S32 dnq_net_set_dns(U32 dnsaddr);
S32 dnq_net_set_dns2(U32 dnsaddr1, U32 dnsaddr2);
S32 dnq_net_set_gw_addr(U8 *if_name, U32 gw);
U32 dnq_net_get_gw_addr(U8 *if_name);
S32 dnq_net_set_mask(U8 *if_name, U32 submask);
U32 dnq_net_get_mask(U8 *if_name);
S32 dnq_net_set_broad_addr(U8 *if_name, U32 brdaddr);
U32 dnq_net_get_broad_addr(U8 *if_name);
S32 dnq_net_set_ipaddr(U8 *if_name, U32 ip_addr);
U32 dnq_net_get_ipaddr(U8 *if_name);
S32 dnq_net_set_macaddr(U8 *if_name, U8 *mac_addr);
S32 dnq_net_get_macaddr(U8 *if_name, U8 *mac_addr);
S32 dnq_net_get_link_status(U8 *if_name);
U32 dnq_net_get_host_by_name(U8 *cname);
S32 dnq_net_link_isgood();
S32 dnq_network_check();
S32 dnq_network_init();
S32 dnq_network_deinit();


#endif /* _DNQ_NETWORK_H_ */

