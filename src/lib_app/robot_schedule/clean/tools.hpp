#ifndef _UTILS_TOOLS_H
#define _UTILS_TOOLS_H
#include "apptypes.hpp"
#include "stdio.h"
#include "string.h"
#include <sys/time.h>
#include <math.h>

#define abs_val(x) ((x) > 0 ? (x) : -(x))	//  绝对值
// U16 dif(U16 x1, U16 x2);
// U32 square(U32 x);
// double float_sqrt(double A);
// S32 angle_val_chk(S32 val);
// int float_atan(float x0, float y0, float x1, float y1);
// void gyro_to_rect(int angle,int *out);
// void rect_to_gyro(int angle,int *out);
// int get_angle_diff(int angle1, int angle2);



    extern long getTimeStamp();


int get_angle_diff(int angle1, int angle2);
int float_atan(float x0, float y0, float x1, float y1);
long get_dif_time_long(long time);
void get_time(struct timeval *tv);
long get_dif_time(struct timeval *tv);
long speedToFrequency(int speed); //转速转频率
void object_to_byte(char* obj,int len,char* out);
int endianJudge();//大小端模式判断
U16 u8_to_u16(U8 low,U8 high);
U16 crc16check(U8 *data, U32 len);

#endif	//EOF _UTILS_TOOLS_H
