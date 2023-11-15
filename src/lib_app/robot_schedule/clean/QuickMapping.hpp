#ifndef _QUICK_MAPPING_H
#define _QUICK_MAPPING_H

#include "apptypes.hpp"
#include "cmd_interface.hpp"
#include "fsm.hpp"
#include "map.hpp"
void qucik_mapping_init();
void qucik_mapping_perform();
void send_obstacle_to_slam(char left,char right,char lc,char rc,char center,char type);

#endif