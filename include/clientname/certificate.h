#ifndef _CERTIFICATE_H_
#define _CERTIFICATE_H_
#include <stdint.h>

int tls_setup(int fd);
int cert_provision(void);

#endif /* _CERTIFICATE_H_ */