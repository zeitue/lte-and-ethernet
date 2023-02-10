#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_

#include "message.h"

void repository_init();
void repository_send_message(union message msg);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#endif /* _REPOSITORY_H_ */
