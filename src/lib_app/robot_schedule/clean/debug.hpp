#ifndef _DEBUG_H
#define _DEBUG_H
#include <math.h>
#include <stdio.h>
#include "robot_schedule/zlog/zlog.h"

//#define LOG_CONFIG	"/userdata/lds.conf"
#define LOG_CONFIG	"/etc/lds.conf"

int debug_zlog_init(void);
extern zlog_category_t *zc;
#define ZLOG  2
#define ON    1
#define OFF   0

/* 控制端打印开关: 0关，非0开 */
#define LOG_PRINT  ZLOG

/* log调试打印 */
#define LOG_DEBUG(fmt, ...)                                                               \
    do                                                                                    \
    {                                                                                     \
        if (LOG_PRINT == ON)                                                                    \
        {                                                                                 \
            printf("[DEBUG][%s,%d]: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                 \
        else if(LOG_PRINT == ZLOG)														  \
        {																				  \
			zlog_debug(zc, fmt,  ##__VA_ARGS__ ); 									\
		}																				  \
    } while (0)

#define LOG_INFO(fmt, ...)                                                               \
    do                                                                                    \
    {                                                                                     \
        if (LOG_PRINT == ON)                                                                    \
        {                                                                                 \
            printf("[INFO][%s,%d]: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                 \
        else if(LOG_PRINT == ZLOG)														  \
        {																				  \
			zlog_info(zc, fmt,  ##__VA_ARGS__ );									  \
		}			\
    } while (0)

#define LOG_ERROR(fmt, ...)                                                               \
    do                                                                                    \
    {                                                                                     \
        if (LOG_PRINT == ON)                                                                    \
        {                                                                                 \
            printf("[ERROR][%s,%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        }                                                                                 \
        else if(LOG_PRINT == ZLOG)														  \
        {																				  \
			zlog_error(zc, fmt ,  ##__VA_ARGS__ );								  \
		}	\
    } while (0)
#endif		
