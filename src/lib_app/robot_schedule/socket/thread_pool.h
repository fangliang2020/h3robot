#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <pthread.h>


//线程控制
extern pthread_mutex_t mut_navi_conn;//连接监听
extern pthread_mutex_t mut_navi_pose;//接收POSE
extern pthread_mutex_t mut_navi_imu;//上传ANGLE+CODE
extern pthread_mutex_t mut_navi_odom;	//上传odom
extern pthread_mutex_t mut_ir;//墙检和地检

extern pthread_cond_t cond_navi_conn;
extern pthread_cond_t cond_navi_pose;
extern pthread_cond_t cond_navi_imu;
extern pthread_cond_t cond_navi_odom;
extern pthread_cond_t cond_ir;
//线程状态
enum thread_status{
	STOP =0,
	RUN =1,
};
extern int status_navi_conn,status_navi_pose,status_navi_imu,status_navi_odom,status_ir;

void thread_resume(int id);
void thread_pause(int id);


#endif
