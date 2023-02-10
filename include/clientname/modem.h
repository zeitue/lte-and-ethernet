#ifndef _MODEM_H_
#define _MODEM_H_
#include <stdint.h>

void modem_init();
void modem_connect();
int32_t modem_get_temperature();
int modem_get_iccid(char *result, int result_len);
int modem_get_imei(char *result, int result_len);
int modem_get_uuid(char *result, int result_len);

#endif /* _MODEM_H_ */