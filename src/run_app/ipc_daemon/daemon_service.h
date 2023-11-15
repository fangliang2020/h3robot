

#ifndef __DAEMON_SEVICE_H__
#define __DAEMON_SEVICE_H__

// #ifdef __cplusplus
// extern "C" {
// #endif


int daemon_services_init();
int daemon_services_start(unsigned int timer_ms);
int daemon_services_stop(void);

// #ifdef __cplusplus
// }
// #endif

#endif
