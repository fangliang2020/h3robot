#ifndef _APP_MSGQUEUE_H
#define _APP_MSGQUEUE_H
#include <stdio.h>  /* printf, scanf, NULL */
#include <stdlib.h>  /* malloc, free, rand, system */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "serialize.h"
//对于一般小数据,频率高的数据通过消息队列处理,大数据和频率低的数据直接通过socket
//目前只接收IMU和ODOM数据上报
pthread_mutex_t static g_enqueue_lock;
pthread_mutex_t static g_dequeue_lock;

typedef enum msg_type{
	MAP_TYPE=0,	
	POSE_TYPE,
	ZONE_TYPE,
	IMU_TYPE,
	ODOM_TYPE,
	CMD_TYPE,
	NULL_TYPE
}MSG_TYPE;
#define MSG_MAX_LEN 1024
typedef struct msg_struct{
	U8 id;						//消息ID号
    U8 type;					//消息类型
    char data[MSG_MAX_LEN];    	//消息数据[最大长度]
}MSG_STRUCT;
typedef struct queue_node{	
	MSG_STRUCT node;
	struct queue_node *next;   	//后一节点	
}QUEUE_NODE;
typedef struct msg_queue
{
	const char* queue_name;	//队列名称
    QUEUE_NODE *front;	//头部节点,找到top就能读取头部节点
    QUEUE_NODE *rear;   //尾部节点,找到rear就能向队列末尾添加节点  
}MSGQUEUE;

void init_queue(MSGQUEUE* que);
void enqueue(MSGQUEUE* que, MSG_STRUCT msg);
MSG_STRUCT dequeue(MSGQUEUE* que);
int queue_empty(MSGQUEUE* que);
void print_queue(MSGQUEUE* que);
#endif