#ifndef _CMD_INTERFACE_H
#define _CMD_INTERFACE_H
//#include "../global/types.h"
//#include "../global/tools.h"
//#include "../global/debug.h"
#include "fsm.hpp"
#include "zmode.hpp"
#include "followwall.hpp"
extern U8 tof_msg;
#define TOF_FRONT (tof_msg >> 3 & 1)

#define TOF_LEFT_FRONT (tof_msg >> 4 & 1)
#define TOF_LEFT (tof_msg >> 5 & 1)
#define TOF_MORE_LEFT (tof_msg >> 6 & 1)

#define TOF_RIGHT_FRONT (tof_msg >> 2 & 1)
#define TOF_RIGHT (tof_msg >> 1 & 1)
#define TOF_MORE_RIGHT (tof_msg >> 0 & 1)   



extern Point *global_path_;
extern int global_path_count_;
extern bool global_path_update_;
extern Point current_pose_;
extern double current_angle_;

void tof_msg_analyze(U8 cmd);
void cmd_from_asr(U8 cmd);
void path_download(Point path[],int path_count);
void free_path();
void cmd_publish(uint8_t cmd);

#endif