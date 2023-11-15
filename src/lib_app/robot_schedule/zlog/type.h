
#ifndef __type_h
#define __type_h
/* record is user-defined output function and it's name from configure file */
typedef struct zlog_msg_s {
	char *buf;
	size_t len;
	char *path;
} zlog_msg_t; /* 3 of this first, see need thread or not later */

typedef int (*zlog_record_fn)(zlog_msg_t * msg);
#endif