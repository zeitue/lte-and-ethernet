#include <date_time.h>
#include <clientname/certificate.h>
#include <clientname/modem.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME modem
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* LTE connection */
static K_SEM_DEFINE(lte_connected, 0, 1);

static void lte_event_handler(const struct lte_lc_evt *const evt) {
  switch (evt->type) {
    case LTE_LC_EVT_NW_REG_STATUS:
      if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
          (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
        LOG_INF("Connected to LTE");
        k_sem_give(&lte_connected);
      }
      break;
    default:
      break;
  }
}

int modem_get_iccid(char *result, int result_len) {
  char p[60] = {0};
  if (result == NULL || result_len < 30) {
    LOG_ERR("buffer not big enough for iccid");
    return -1;
  }
  nrf_modem_at_cmd(p, sizeof(p), "AT+CRSM=176,12258,0,0,10");
  char *start = strchr(p, '"');
  if (start == NULL) {
    LOG_ERR("invalid response from modem");
    return -1;
  }

  char *end = strchr(start + 1, '"');
  if (end == NULL) {
    LOG_ERR("invalid response from modem");
    return -1;
  }

  memcpy(result, start + 1, end - start - 1);
  return 0;
}

int modem_get_imei(char *result, int result_len) {
  if (result == NULL || result_len < 30) {
    LOG_ERR("buffer not big enough for imei");
    return -1;
  }
  int err = nrf_modem_at_cmd(result, result_len, "AT+CGSN");
  char *end = strchr(result, '\r');
  if (end != NULL) {
    result[end - result] = '\0';
  }
  return err;
}

int modem_get_uuid(char *result, int result_len) {
  char p[32] = {0};
  if (modem_get_imei(p, sizeof(p)) != 0) {
    LOG_ERR("failed to get imei so cannot build uuid");
    return -1;
  }
  int p_len = strlen(p);
  int h = 0;
  for (int i = 0; i < p_len; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      result[(i * 2) + h] = '-';
      h++;
    }
    result[(i * 2) + h] = '0';
    result[(i * 2) + h + 1] = p[i];
  }
  return 0;
}

int32_t modem_get_temperature() {
  char res[32] = {0};
  int32_t temp = 0;
  nrf_modem_at_cmd(res, sizeof(res), "AT%%XTEMP?");

  char *found = strchr(res, ':') + 1;

  if (found != NULL) {
    temp = strtol(found + 1, NULL, 10);
  } else {
    LOG_WRN("invalid temperature information");
  }
  return temp;
}

static void date_time_evt_handler(const struct date_time_evt *evt) {
  /* TODO date time event handling */
  LOG_INF("got network datetime");
}

void modem_init() {
  /* handle datetime*/
  if (IS_ENABLED(CONFIG_DATE_TIME)) {
    date_time_register_handler(date_time_evt_handler);
  }

  /* Put the modem in normal operating mode*/
  nrf_modem_lib_init(NORMAL_MODE);

  /* Provision certifcates */
  int err = cert_provision();
  if (err != 0) {
    LOG_ERR("Cannot add certificates");
  }
  /* initialize the modem*/
  lte_lc_init();
  lte_lc_register_handler(lte_event_handler);

  /* Disable PSM. (power saving) */
  lte_lc_psm_req(true);
}

void modem_connect() {
  lte_lc_connect();

  k_sem_take(&lte_connected, K_FOREVER);
}