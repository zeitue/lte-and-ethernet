#include <errno.h>
#include <clientname/ethernet.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_config.h>

LOG_MODULE_REGISTER(eth, LOG_LEVEL_DBG);


static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
                    struct net_if *iface) {
  int i = 0;

  if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
    return;
  }

  for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
    char buf[NET_IPV4_ADDR_LEN];

    if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP) {
      continue;
    }

    LOG_INF("Your address: %s",
            net_addr_ntop(AF_INET,
                          &iface->config.ip.ipv4->unicast[i].address.in_addr,
                          buf, sizeof(buf)));
    LOG_INF("Lease time: %u seconds", iface->config.dhcpv4.lease_time);
    LOG_INF("Subnet: %s",
            net_addr_ntop(AF_INET, &iface->config.ip.ipv4->netmask, buf,
                          sizeof(buf)));
    LOG_INF("Router: %s", net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
                                        buf, sizeof(buf)));
  }
}

void eth_init(void) {
  struct net_if *iface;

  LOG_INF("Run dhcpv4 client");

  net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
  net_mgmt_add_event_callback(&mgmt_cb);

  iface = net_if_get_default();

  net_dhcpv4_start(iface);

}