#if __has_include("clientname/build.h")
#include <clientname/build.h>
#endif
#include <clientname/ethernet.h>
#include <clientname/modem.h>
#include <clientname/repository.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if IS_ENABLED(CONFIG_NET_L2_ETHERNET)
#include <zephyr/net/net_if.h>
#endif

void main() {
#if defined(VERSION) && defined(BUILD)
  printk("Version: %s\nBuild: %s\n", VERSION, BUILD);
#endif

  eth_init();

  /* Enable modem */
  modem_init();

  /* connect to LTE */
  // modem_connect();

  /* Enable repositoy*/
  k_sleep(K_SECONDS(10));
  repository_init();
}
