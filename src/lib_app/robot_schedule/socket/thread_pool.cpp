#include<stdio.h>      /*标准输入输出定义*/
#include<stdlib.h>     /*标准函数库定义*/
#include<unistd.h>     /*Unix标准函数定义*/
#include<sys/types.h>  /**/
#include<sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <string.h>
//#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>  
#include <signal.h> 
#include "thread_pool.h"

pthread_mutex_t mut_navi_conn  	= PTHREAD_MUTEX_INITIALIZER;	//连接监听
pthread_mutex_t mut_navi_pose  	= PTHREAD_MUTEX_INITIALIZER;	//接收POSE
pthread_mutex_t mut_navi_imu   	= PTHREAD_MUTEX_INITIALIZER;	//上传ANGLE
pthread_mutex_t mut_navi_odom   = PTHREAD_MUTEX_INITIALIZER;	//上传ODOMs
pthread_mutex_t mut_ir 	  		= PTHREAD_MUTEX_INITIALIZER;	//墙检和地检

pthread_cond_t cond_navi_conn 	= PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_navi_pose 	= PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_navi_imu 	= PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_navi_odom 	= PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_ir 			= PTHREAD_COND_INITIALIZER;

int status_navi_conn =RUN;
int status_navi_pose =RUN;
int status_navi_imu = RUN;
int status_navi_odom = RUN;
int status_ir = RUN;
void thread_resume(int id)
{
	switch(id){
		case 1:		//navi_conn
		    if (status_navi_conn == STOP)
		    {  
		        pthread_mutex_lock(&mut_navi_conn);
		        status_navi_conn = RUN;
		        pthread_cond_signal(&cond_navi_conn);
		        printf("pthread navi pose run!\n");
		        pthread_mutex_unlock(&mut_navi_conn);
		    }  
		    else
		    {  
		        printf("pthread navi pose already run\n");
		    } 			
			break;		
		case 2:		//navi_pose
		    if (status_navi_pose == STOP)
		    {  
		        pthread_mutex_lock(&mut_navi_pose);
		        status_navi_pose = RUN;
		        pthread_cond_signal(&cond_navi_pose);
		        printf("pthread navi pose run!\n");
		        pthread_mutex_unlock(&mut_navi_pose);
		    }  
		    else
		    {  
		        printf("pthread navi pose already run\n");
		    } 			
			break;
		case 3:		//imu
		    if (status_navi_imu == STOP)
		    {  
		        pthread_mutex_lock(&mut_navi_imu);
		        status_navi_imu = RUN;
		        pthread_cond_signal(&cond_navi_imu);
		        printf("pthread imu run!\n");
		        pthread_mutex_unlock(&mut_navi_imu);
		    }  
		    else
		    {  
		        printf("pthread imu already run\n");
		    } 
			break;  
		case 4:		//odom
		    if (status_navi_odom == STOP)
		    {  
		        pthread_mutex_lock(&mut_navi_odom);
		        status_navi_odom = RUN;
		        pthread_cond_signal(&cond_navi_odom);
		        printf("pthread odom run!\n");
		        pthread_mutex_unlock(&mut_navi_odom);
		    }  
		    else
		    {  
		        printf("pthread odom already run\n");
		    } 
			break;  			
		case 5:		//ir
		    if (status_ir == STOP)
		    {  
		        pthread_mutex_lock(&mut_ir);
		        status_ir = RUN;
		        pthread_cond_signal(&cond_ir);
		        printf("pthread ir run!\n");
		        pthread_mutex_unlock(&mut_ir);
		    }  
		    else
		    {  
		        printf("pthread ir already run\n");
		    } 
			break;
	}
 
}
 
 
void thread_pause(int id)
{
	switch(id){
		case 1:	//navi_conn
			if (status_navi_conn == RUN)
			{  
				pthread_mutex_lock(&mut_navi_conn);
				status_navi_conn = STOP;
				printf("thread conn stop!\n");
				pthread_mutex_unlock(&mut_navi_conn);
			}  
			else
			{  
				printf("pthread conn pause already\n");
			}
			break;		
		case 2:	//navi_pose
			if (status_navi_pose == RUN)
			{  
				pthread_mutex_lock(&mut_navi_pose);
				status_navi_pose = STOP;
				printf("thread pose stop!\n");
				pthread_mutex_unlock(&mut_navi_pose);
			}  
			else
			{  
				printf("pthread pose pause already\n");
			}
			break;
		case 3:	//imu
			if (status_navi_imu == RUN)
			{  
				pthread_mutex_lock(&mut_navi_imu);
				status_navi_imu = STOP;
				printf("thread imu stop!\n");
				pthread_mutex_unlock(&mut_navi_imu);
			}  
			else
			{  
				printf("pthread imu pause already\n");
			}
			break;
		case 4:	//odom
			if (status_navi_odom == RUN)
			{  
				pthread_mutex_lock(&mut_navi_odom);
				status_navi_odom = STOP;
				printf("thread odom stop!\n");
				pthread_mutex_unlock(&mut_navi_odom);
			}  
			else
			{  
				printf("pthread odom pause already\n");
			}
			break;			
		case 5:	//ir
			if (status_ir == RUN)
			{  
				pthread_mutex_lock(&mut_ir);
				status_ir = STOP;
				printf("thread ir stop!\n");
				pthread_mutex_unlock(&mut_ir);
			}  
			else
			{  
				printf("pthread ir pause already\n");
			}

			break;


	}
}
