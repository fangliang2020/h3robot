//
//  smartplan.h
//  lanclient
//
//  Created by chzns on 2018/5/7.
//  Copyright © 2018年 chznsrudp. All rights reserved.
//

#ifndef smartplan_h
#define smartplan_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include "protocol.h"

#define JOIN_WAKEUP 1
    
typedef struct _lanclient lanclient;
typedef struct _lanserver lanserver;
    

typedef struct _wakeup_t  wakeup_t;
typedef struct _lansession lansession;
typedef int (* WakeupCommandEvent)(wakeup_t *w, lansession *device,uint16_t nid, void *data, size_t size, void *arg);
typedef void (*WakeupSessionChangedEvent)(wakeup_t *w, lansession *device, int added);
    
typedef struct wakeupData_t {
    wakeup_t *wakeup;
    WakeupCommandEvent on_command;
    WakeupSessionChangedEvent on_session_changed;
} wakeupData_t;
    
typedef struct _smartplan_t {
    protocol_t proto;
    lanclient *client;
    lanserver *server;
    wakeupData_t wpdata;
} smartplan_t;

smartplan_t *smartp_lan_create();
void smartp_lan_destroy(smartplan_t *lanp);
void smartp_lan_set_prop(smartplan_t *lanp, int prop);
    
#ifdef __cplusplus
}
#endif

#endif /* smartplan_h */
