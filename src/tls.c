
#include <clientname/certificate.h>
#include <modem/modem_key_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

#define LOG_MODULE_NAME tls
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define TLS_SEC_TAG 40
#define CERT_COUNT 3

static const char *cert[CERT_COUNT] = {
#include "../cert/DigiCertGlobalRootCA.pem"
, /* Don't remove this */
#include "../cert/lets-encrypt-r3.pem"
, /* Don't remove this */
#include "../cert/origin_ca_ecc_root.pem"
};

int cert_provision(void) {
  int err;
  bool exists;
  int mismatch;

  for (int i = 0; i < CERT_COUNT; i++) {
    err = modem_key_mgmt_exists(i + TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                                &exists);
    if (err) {
      LOG_ERR("Failed to check for certificates err %d", err);
      continue;
    }

    if (exists) {
      mismatch = modem_key_mgmt_cmp(
          i + TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert[i], strlen(cert[i]));
      if (!mismatch) {
        LOG_INF("Certificate match");
        continue;
      }

      printk("Certificate mismatch");
      err =
          modem_key_mgmt_delete(i + TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
      if (err) {
        LOG_WRN("Failed to delete existing certificate, err %d", err);
      }
    }

    LOG_INF("Provisioning certificate");

    /*  Provision certificate to the modem */
    err = modem_key_mgmt_write(i + TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                               cert[i], strlen(cert[i]));
    if (err) {
      LOG_ERR("Failed to provision certificate, err %d", err);
      continue;
    }
  }

  return 0;
}

int tls_setup(int fd) {
  int err;
  int verify;

  /* Security tag that we have provisioned the certificate with */
  const sec_tag_t tls_sec_tag[] = {
      TLS_SEC_TAG,
      TLS_SEC_TAG + 1,
      TLS_SEC_TAG + 2,
  };

  /* Set up TLS peer verification */
  enum {
    NONE = 0,
    OPTIONAL = 1,
    REQUIRED = 2,
  };

  verify = OPTIONAL;

  err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
  if (err) {
    LOG_ERR("Failed to setup peer verification, err %d\n", errno);
    return err;
  }

  err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
                   sizeof(tls_sec_tag));
  if (err) {
    LOG_ERR("Failed to setup TLS sec tag, err %d", errno);
    return err;
  }

  err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, CONFIG_REPOSITORY_HOSTNAME,
                   sizeof(CONFIG_REPOSITORY_HOSTNAME) - 1);
  if (err) {
    LOG_ERR("Failed to setup TLS hostname, err %d", errno);
    return err;
  }
  return 0;
}