#include "navigate.h"
#include <pthread.h>
#include "robot_schedule/schedule/NetserverInstruct.h"
MSGQUEUE *c_read_queue;
MSGQUEUE *c_write_queue;
//int gAngle; //全局陀螺仪角度,范围[-180,+180],-180为X负,+180为X正;+90为Y正;-90为Y负
//Point * coordinates;
//由于imu和odom数据频率比较高,采用2个socket分别处理
int socket_imu_id =-1,socket_odom_id =-1,
	socket_pose_id =-1,socket_low_id=-1,socket_dist_id=-1;	//socketid
int socket_imu_conn =0,socket_odom_conn =0,
	socket_pose_conn=0,socket_low_conn = 0,socket_dist_conn=0;	//connect state


int client_imu_connect();
int client_odom_connect();
int client_low_connect();
int client_dist_connect();

void* check_imu_connect(void* arg);
void* check_odom_connect(void* arg);
void* check_low_connect(void* arg);
void* check_dist_connect(void* arg);

int client_imu_send(char* buf,int len);
int client_odom_send(char* buf,int len);
int client_low_send(char* buf,int len);
int client_dist_send(char* buf,int len);
U16 u8_to_u16(U8 front,U8 behind);
U16 crc16check(char *data, U32 len);
U16 crc16check8(U8 *data, U32 len);
long getTimeStamp();
//void pose_download(int x,int y,double angle);


//将沿边清扫数据发送到导航,由followwall调用
void followwall_maps_upload(BLOCK_DATA *data,int len){
	printf("followwall_maps_upload:%d",len);
	//LOG_INFO("checkcode:%d",checkcode);
	char buffer[50000]={0};
	int alen=0,blen=0,clen=0,dlen=0;
	U16 head = u8_to_u16(MAP_DATA_HEAD_CHAR2,MAP_DATA_HEAD_CHAR1);	
	serialize_short(head,buffer,&alen);		//头部序列化,目的是生成校验码	
	int blength = BLOCK_LEN*len;
	serialize_int(blength,&buffer[alen],&blen);
	serialize_blocks_data(data,len,&buffer[alen+blen],&clen);//blocks序列化
	U16 checkcode= crc16check(buffer,alen + blen + clen);	
	//LOG_INFO("checkcode:%d",checkcode);
	serialize_short(checkcode,&buffer[alen + blen + clen],&dlen);//校验码序列化
	//LOG_INFO("a:%d,b:%d,c:%d,d:%d,maps_len:%d,checkcode:%d",
			//alen,blen,clen,dlen,(alen+blen+clen+dlen),checkcode);
	// for(int i =0;i<alen+blen+clen+dlen;i++){
	// 	LOG_INFO("followwall_maps_upload_msg:%x",buffer[i]);
	// }	
	printf("map_end\n");
	client_low_send(buffer,alen+blen+clen+dlen);

}

//获取开始点到结束点的路径
void get_route(Point start,Point end){
	printf("get_route:[%d,%d],[%d,%d]",start.x,start.y,end.x,end.y);
	char buffer[200]={0};
	int alen=0,blen=0,clen=8,dlen=0;
	U16 head = u8_to_u16(PATH_HEAD_CHAR2,PATH_HEAD_CHAR1);
	serialize_short(head,buffer,&alen);		//头部序列化,目的是生成校验码
	serialize_point_data(&start,&buffer[alen],&blen);
	serialize_point_data(&end,&buffer[alen+blen],&clen);
	U16 checkcode= crc16check(buffer,alen + blen+ clen);		
	serialize_short(checkcode,&buffer[alen + blen+ clen],&dlen);//校验码序列化
	for(int i =0;i<alen+blen+clen+dlen;i++){
		printf("get_route:%x",buffer[i]);
	}		
	client_low_send(buffer,alen+blen+clen+dlen);
}

//陀螺仪数据回调,对象字节化后放入c_write_queue
void imu_callback(double angleX,double angleY,double angleZ,
	double accX,double accY,double accZ){	
	char buffer[200]={0};
	int alen=0,blen=0,clen=8,dlen=0;
	U16 head = u8_to_u16(IMU_DATA_HEAD_CHAR2,IMU_DATA_HEAD_CHAR1);
	serialize_short(head,buffer,&alen);		//头部序列化,目的是生成校验码
	IMU_DATA imu={angleX,angleY,angleZ,accX,accY,accZ};
	serialize_imu_data(&imu,&buffer[alen],&blen);//IMU序列化,目的是生成校验码
	long timestamp = getTimeStamp();
	memcpy(&buffer[alen + blen],&timestamp,clen);//时间戳	
	U16 checkcode= crc16check(buffer,alen + blen+ clen);		
	serialize_short(checkcode,&buffer[alen + blen+ clen],&dlen);//校验码序列化
//	LOG_INFO("imu_callback:[%lf,%lf,%lf],[%lf,%lf,%lf],%ld",
//			angleX,angleY,angleZ,accX,accY,accZ,timestamp);
	client_imu_send(buffer,alen+blen+clen+dlen);
}
//轮子码盘数据回调,参数：左右轮子速度mm/s
void odom_callback(int speed_left,int speed_right){	
	char buffer[200]={0};
	int alen=0,blen=0,clen=8,dlen=0;
	U16 head = u8_to_u16(ODOM_DATA_HEAD_CHAR2,ODOM_DATA_HEAD_CHAR1);
	serialize_short(head,buffer,&alen);		//头部序列化,目的是生成校验码
	ODOM_DATA odom = {speed_left,speed_right};
	serialize_odom_data(&odom,&buffer[alen],&blen);//ODOM序列化,目的是生成校验码
	long timestamp = getTimeStamp();
	memcpy(&buffer[alen + blen],&timestamp,clen);//时间戳	
	U16 checkcode= crc16check(buffer,alen + blen + clen);		
	serialize_short(checkcode,&buffer[alen + blen + clen],&dlen);//校验码序列化
//	LOG_INFO("odom_callback:%d,%d,%ld",speed_left,speed_right,timestamp);
	client_odom_send(buffer,alen+blen+clen+dlen);
	//calculate_vel(speed_left,speed_right);
}

//控制命令汇总
void send_cmd(U8 cmd){
	printf("send_cmd:%d",cmd);
	char buffer[200]={0};
	int alen=0,blen=0;
	U16 head = u8_to_u16(CMD_HEAD_CHAR2,CMD_HEAD_CHAR1);
	serialize_short(head,buffer,&alen);
	U8 cmdx = cmd;
	memcpy(&buffer[2],&cmdx,1);
	U16 checkcode= crc16check(buffer,3);
	serialize_short(checkcode,&buffer[3],&blen);
	int total_len = alen+1+blen;
	printf("cmd_len:%d,checkcode:%d",total_len,checkcode);	
//	for(int i =0;i<total_len;i++){
//		LOG_INFO(":%x",buffer[i]);
//	}
	client_low_send(buffer,total_len);
}

//关闭连接
void client_imu_close(){
	close(socket_imu_id);
	socket_imu_conn =0;
	printf("client_imu_close");
}
void client_odom_close(){
	close(socket_odom_id);
	socket_odom_conn =0;
	printf("client_odom_close");
}
void client_pose_close(){
	close(socket_pose_id);
	socket_pose_conn =0;
	printf("client_pose_close");
}
void client_low_close(){
	close(socket_low_id);
	socket_low_conn =0;
	printf("client_low_close");
}
void client_dist_close(){
	close(socket_dist_id);
	socket_dist_conn =0;
	printf("client_dist_close");
}



//连接
int client_imu_connect(){ 
	//printf("client_imu_connect\n");
	int iret = -1;
	if((socket_imu_id = socket(PF_UNIX,SOCK_STREAM, 0))==-1){	
		usleep(1000);
		socket_imu_conn=0;
		iret =-2;			
	}else{
/**		
		//查看系统socket缓存大小
		int defRcvBufSize = -1;
		socklen_t optlen = sizeof(defRcvBufSize);
 		if (getsockopt(socket_imu_id, SOL_SOCKET, SO_RCVBUF, &defRcvBufSize, &optlen) < 0)
     	{
         	LOG_INFO("getsockopt error=%d(%s)!!!", errno, strerror(errno));
     	}
     	LOG_INFO("OS default tcp socket recv buff size is: %d", defRcvBufSize);		
		//重置系统缓存大小
		int rcvBufSize =500000;
		optlen = sizeof(rcvBufSize);
		if (setsockopt(socket_imu_id, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, optlen) < 0)
     	{
         	LOG_INFO("setsockopt error=%d(%s)!!!", errno, strerror(errno));
     	}
     	LOG_INFO("set tcp socket(%d) recv buff size to %d OK!!!", socket_imu_id, rcvBufSize); 				
**/
		//设置超时
		struct timeval ti;
		ti.tv_sec = 1;
		ti.tv_usec =0;		
		if(setsockopt(socket_imu_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1)
		{
			printf("setsockopt_error");
			usleep(1000);
			socket_imu_conn=0;
			client_imu_close();
			iret =-3;
		}else{
			struct sockaddr_un servaddr;
			servaddr.sun_family=AF_UNIX;	//本地无名套接字
			strcpy(servaddr.sun_path,UNIX_DOMAIN_NAVI_IMU);		
			if(connect(socket_imu_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{					
				if(errno == EINPROGRESS){	//peer正在处理中

				}else{	//连接出错了,需要关闭重连

				}
//				LOG_INFO("%s connect fail:%d",UNIX_DOMAIN_NAVI_IMU,errno);
				socket_imu_conn=0;
				usleep(1000);
				iret =-4;
			}else{
				printf("connect_ok:%s",UNIX_DOMAIN_NAVI_IMU);
				socket_imu_conn=1;
				iret =1;
			}
		}
	}
//	LOG_INFO("iret:%d",iret);
	return iret;
}

int client_odom_connect(){ 
	//printf("client_odom_connect\n");
	int iret = -1;
	if((socket_odom_id = socket(PF_UNIX,SOCK_STREAM, 0))==-1){	
		usleep(1000);
		socket_odom_conn=0;
		iret =-2;			
	}else{
/**		
		//查看系统socket缓存大小
		int defRcvBufSize = -1;
		socklen_t optlen = sizeof(defRcvBufSize);
 		if (getsockopt(socket_odom_id, SOL_SOCKET, SO_RCVBUF, &defRcvBufSize, &optlen) < 0)
     	{
         	LOG_INFO("getsockopt error=%d(%s)!!!", errno, strerror(errno));
     	}
     	LOG_INFO("OS default tcp socket recv buff size is: %d", defRcvBufSize);		
		//重置系统缓存大小
		int rcvBufSize =500000;
		optlen = sizeof(rcvBufSize);
		if (setsockopt(socket_odom_id, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, optlen) < 0)
     	{
         	LOG_INFO("setsockopt error=%d(%s)!!!", errno, strerror(errno));
     	}
     	LOG_INFO("set tcp socket(%d) recv buff size to %d OK!!!", socket_odom_id, rcvBufSize); 				
**/
		//设置超时
		struct timeval ti;
		ti.tv_sec = 1;
		ti.tv_usec =0;		
		if(setsockopt(socket_odom_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1)
		{
			printf("setsockopt_error");
			usleep(1000);
			socket_odom_conn=0;
			client_odom_close();
			iret =-3;
		}else{
			struct sockaddr_un servaddr;
			servaddr.sun_family=AF_UNIX;	//本地无名套接字
			strcpy(servaddr.sun_path,UNIX_DOMAIN_NAVI_ODOM);		
			if(connect(socket_odom_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{					
				if(errno == EINPROGRESS){	//peer正在处理中

				}else{	//连接出错了,需要关闭重连

				}
//				LOG_INFO("%s connect fail:%d",UNIX_DOMAIN_NAVI_ODOM,errno);
				socket_odom_conn=0;
				usleep(1000);
				iret =-4;
			}else{
				printf("connect_ok:%s",UNIX_DOMAIN_NAVI_ODOM);
				socket_odom_conn=1;
				iret =1;
			}
		}
	}
//	LOG_INFO("iret:%d",iret);
	return iret;
}

int client_pose_connect(){ 
	int iret = -1;
	if((socket_pose_id = socket(PF_UNIX,SOCK_STREAM, 0))==-1){	
		usleep(1000);
		socket_pose_conn=0;
		iret =-2;			
	}else{				
		//设置超时
		struct timeval ti;
		ti.tv_sec = 1;
		ti.tv_usec =0;		
		if(setsockopt(socket_pose_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1)
		{
			printf("setsockopt_error");
			usleep(1000);
			socket_pose_conn=0;
			client_pose_close();
			iret =-3;
		}else{
			struct sockaddr_un servaddr;
			servaddr.sun_family=AF_UNIX;	//本地无名套接字
			strcpy(servaddr.sun_path,UNIX_DOMAIN_NAVI_POSE);		
			if(connect(socket_pose_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{					
				if(errno == EINPROGRESS){	//peer正在处理中

				}else{	//连接出错了,需要关闭重连

				}
//				LOG_INFO("%s connect fail:%d",UNIX_DOMAIN_NAVI_POSE,errno);
				socket_pose_conn=0;
				usleep(1000);
				iret =-4;
			}else{
				printf("connect_ok:%s",UNIX_DOMAIN_NAVI_POSE);
				socket_pose_conn=1;
				iret =1;
			}
		}
	}
//	LOG_INFO("iret:%d",iret);
	return iret;
}
int client_low_connect(){ 
	int iret = -1;
	if((socket_low_id = socket(PF_UNIX,SOCK_STREAM, 0))==-1){	
		usleep(1000);
		socket_low_conn=0;
		iret =-2;			
	}else{				
		//设置超时
		struct timeval ti;
		ti.tv_sec = 1;
		ti.tv_usec =0;		
		if(setsockopt(socket_low_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1)
		{
			printf("setsockopt_error");
			usleep(1000);
			socket_low_conn=0;
			client_low_close();
			iret =-3;
		}else{
			struct sockaddr_un servaddr;
			servaddr.sun_family=AF_UNIX;	//本地无名套接字
			strcpy(servaddr.sun_path,UNIX_DOMAIN_NAVI_LOW);		
			if(connect(socket_low_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{					
				if(errno == EINPROGRESS){	//peer正在处理中

				}else{	//连接出错了,需要关闭重连

				}
//				LOG_INFO("%s connect fail:%d",UNIX_DOMAIN_NAVI_LOW,errno);
				socket_low_conn=0;
				usleep(1000);
				iret =-4;
			}else{
				printf("connect_ok:%s",UNIX_DOMAIN_NAVI_LOW);
				socket_low_conn=1;
				iret =1;
			}
		}
	}
//	LOG_INFO("iret:%d",iret);
	return iret;
}
int client_dist_connect(){ 
	int iret = -1;
	if((socket_dist_id = socket(PF_UNIX,SOCK_STREAM, 0))==-1){	
		usleep(1000);
		socket_dist_conn=0;
		iret =-2;			
	}else{				
		//设置超时
		struct timeval ti;
		ti.tv_sec = 1;
		ti.tv_usec =0;		
		if(setsockopt(socket_dist_id,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti)) == -1)
		{
			printf("setsockopt_error");
			usleep(1000);
			socket_dist_conn=0;
			client_dist_close();
			iret =-3;
		}else{
			struct sockaddr_un servaddr;
			servaddr.sun_family=AF_UNIX;	//本地无名套接字
			strcpy(servaddr.sun_path,UNIX_DOMAIN_NAVI_DIST);		
			if(connect(socket_dist_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
			{					
				if(errno == EINPROGRESS){	//peer正在处理中

				}else{	//连接出错了,需要关闭重连

				}
				printf("%s connect fail:%d",UNIX_DOMAIN_NAVI_DIST,errno);
				socket_dist_conn=0;
				usleep(1000);
				iret =-4;
			}else{
				printf("connect_ok:%s",UNIX_DOMAIN_NAVI_DIST);
				socket_dist_conn=1;
				iret =1;
			}
		}
	}
//	LOG_INFO("iret:%d",iret);
	return iret;
}

void* check_connect(void* arg){
	while(1){
		pthread_mutex_lock(&mut_navi_conn);
		while (!status_navi_conn)
		{
	    	pthread_cond_wait(&cond_navi_conn, &mut_navi_conn);
		}
		pthread_mutex_unlock(&mut_navi_conn);
		if(!socket_imu_conn){
//			LOG_INFO("imu_reconnecting...");
			client_imu_connect();
		}
		if(!socket_odom_conn){
//			LOG_INFO("odom_reconnecting...");
			client_odom_connect();
		}
		if(!socket_pose_conn){
//			LOG_INFO("pose_reconnecting...");
			client_pose_connect();
		}
		if(!socket_low_conn){
//			LOG_INFO("low_reconnecting...");
			client_low_connect();
		}
		sleep(1); 
	}

}


void* check_imu_connect(void* arg){
	while(1){
		if(!socket_imu_conn){
			printf("imu_reconnecting...");
			client_imu_connect();
		}
		sleep(1); 
	}
}
void* check_odom_connect(void* arg){
	while(1){
		if(!socket_odom_conn){
			printf("odom_reconnecting...");
			client_odom_connect();
		}
		sleep(1); 
	}
}
void* check_pose_connect(void* arg){
	while(1){
		if(!socket_pose_conn){
			printf("pose_reconnecting...");
			client_pose_connect();
		}
		sleep(1); 
	}
}
void* check_low_connect(void* arg){
	while(1){
		if(!socket_low_conn){
			printf("low_reconnecting...");
			client_low_connect();
		}
		sleep(1); 
	}
}
void* check_dist_connect(void* arg){
	while(1){
		if(!socket_dist_conn){
			printf("dist_reconnecting...");
			client_dist_connect();
		}
		sleep(1); 
	}
}


void nread(int fd,void* buf,int len){
	int iread =read(fd,buf,len);
	if(iread)
		return;
	if(iread==0){	//对端已经关闭了socket
		printf("peer_closed:%d",errno);
		client_low_close();
	}
	if(iread < 0){
		printf("iread:%d,%d",iread,errno);
		if(iread==-1){
			if(errno == EINTR || errno == EAGAIN ||errno == EWOULDBLOCK){	//系统当前中断了,忽略
				printf("ignore:%d",errno);
			}else{
				printf("read_fail_close");
				client_low_close();
			}
		}
	}
}

void* client_pose_read(void* arg){	
	//小数据变量定义
	U8 msgBuf[DEFAULT_BUFFER_SIZE] = { 0 };	//消息包	
    U8 readBuf[1] = { 0 };  //读1字节
    U8 headBuf[2] = { 0 };  //头部缓存
    U8 contBuf[50] = { 0 };
	//int robot_poseX = 0,robot_poseY = 0,robot_poseYAML = 0;
	int robot_X = -1,robot_Y = -1,robot_YAML = -1;
	int timeout=30;    //超时时间设置为10毫秒间隔
    fd_set fs_read;
    struct timeval time;
	time.tv_sec = timeout / 1000; //秒
    time.tv_usec = 0;    //微秒	
	while(1){
		pthread_mutex_lock(&mut_navi_pose);
		while (!status_navi_pose)
		{
			printf("client_pose_read wait \n");
	    	pthread_cond_wait(&cond_navi_pose, &mut_navi_pose);
		}
		pthread_mutex_unlock(&mut_navi_pose);
		usleep(1000);
//		LOG_INFO("while_client_read");
		if(!socket_pose_conn){
			sleep(1); //等待1秒再连接
			//printf("pose waiting.. \n");
			continue;
		}
		FD_ZERO(&fs_read);     
		FD_SET(socket_pose_id, &fs_read);    
		memset(msgBuf,0,sizeof(msgBuf));  //每次读新包之前清空缓存,重置指针pbuf
		U8* pbuf = msgBuf; 	
        /* 调用select等待套接字变为可读或可写，如果select返回0，那么表示超时 */        
		if(select(socket_pose_id + 1, &fs_read, NULL, NULL, &time)==0){//超时            
            continue;
        };        
		nread(socket_pose_id,readBuf,1);//非阻塞,1次读1个字节		
		//POSE数据
		if (readBuf[0] == POSE_DATA_HEAD_CHAR1) { //机器位姿头1
			//printf("11111111111111111111111\n");
			headBuf[0] = readBuf[0];
			nread(socket_pose_id,readBuf,1);
			//printf("socket_pose_id=%x\n",readBuf[0]);
			if (readBuf[0] == POSE_DATA_HEAD_CHAR2) { //机器位姿头2
				
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				
				//读剩下的字节(ROBOT_POSE+check)
				for(int i =0;i<POSE_PACKAGE_LEN-2;i++){
					//printf("333333333333333\n");
					nread(socket_pose_id,&contBuf[i], 1);
				}
				//nread(socket_pose_id,&contBuf[0], POSE_PACKAGE_LEN-2);
				
				memcpy(pbuf, &contBuf[0], POSE_PACKAGE_LEN-2);								
				U8 pack_code1 = msgBuf[POSE_PACKAGE_LEN-2]; //取低字节
				U8 pack_code2 = msgBuf[POSE_PACKAGE_LEN-1]; //取高字节
				U16 pack_code = u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,POSE_PACKAGE_LEN-2);
				
//				LOG_INFO("pose_checkcode:%d,pack_code:%d,low:%d,hig:%d",checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ //包正确,则取ROBOT_POSE数据
					ROBOT_POSE *pose =(ROBOT_POSE*)&msgBuf[2];
					if(pose!=NULL){
						pose_download(pose->point.x,pose->point.y,pose->yaw); //机器位姿
					
		
					
						 if (abs(pose->point.x - robot_X) >= 4 || abs(pose->point.y - robot_Y) >= 4)
						 {
							// std::cout <<"the point.x" << pose->point.y <<std::endl;
							// std::cout <<"the point.y" << -(pose->point.x) <<std::endl;
							// std::cout <<"the robot_device.x" << pose->robot_device.x <<std::endl;
							// std::cout <<"the robot_device.y" << pose->robot_device.y <<std::endl;
							//NetserverInstruct::getNetInstruct()->pose_down_net(pose->robot_device.x,pose->robot_device.y,pose->robot_angle);
							NetserverInstruct::getNetInstruct()->pose_down_net(pose->point.x,pose->point.y,pose->yaw);
							robot_X = pose->point.x; 
						 	robot_Y = pose->point.y;
	
						}
							

						
					}								
				}else{
					printf("pose_packet_check_error:%d,%d",pack_code,checkcode);
				}				
				continue;									
			}
			else{
				printf("pose_head_err:%x",readBuf[0]);
			}
		}
		//未定义的包
		else{
			if(!socket_pose_conn){
				printf("socket_pose_conn_closed_break!");
				break;
			}
			printf("pose_client_error_package:%x",readBuf[0]);
			usleep(10000);
			continue;			
		}
	}
	return NULL;
}

bool map_receive_finish = false;
void* client_low_read(void* arg){	
	//小数据变量定义
	U8 msgBuf[DEFAULT_BUFFER_SIZE] = { 0 };	//消息包	
    U8 readBuf[1] = { 0 };  //读1字节
    U8 headBuf[2] = { 0 };  //头部缓存
    U8 contBuf[50] = { 0 };
	//地图大数据变量定义
	U8 mapBuf[40000] = { 0 };	//根据服务的长度进行调整
	U8 mapbBuf[40000]={0};//根据服务的长度进行调整
	U8 pathBuf[10000]={0};		//路径缓存
	U8 pathbBuf[10000]={0};
    
	int timeout=30;    //超时时间设置为10毫秒间隔
    fd_set fs_read;
    struct timeval time;
	time.tv_sec = timeout / 1000; //秒
    time.tv_usec = 0;    //微秒	
	while(1){
		usleep(1000);
//		LOG_INFO("while_client_read");
		if(!socket_low_conn){
			sleep(1); //等待1秒再连接
			//printf("low waiting..\n");
			continue;
		}
		FD_ZERO(&fs_read);     
		FD_SET(socket_low_id, &fs_read);    
		memset(msgBuf,0,sizeof(msgBuf));  //每次读新包之前清空缓存,重置指针pbuf
		U8* pbuf = msgBuf; 	
		U8* pmbuf = mapBuf;
		U8* ppbuf = pathBuf;
        /* 调用select等待套接字变为可读或可写，如果select返回0，那么表示超时 */        
		if(select(socket_low_id + 1, &fs_read, NULL, NULL, &time)==0){//超时            
            continue;
        };        
		nread(socket_low_id,readBuf,1);//非阻塞,1次读1个字节		
		//MAP数据
		if (readBuf[0] == MAP_DATA_HEAD_CHAR1) { //栅格头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == MAP_DATA_HEAD_CHAR2) { //栅格头2
				headBuf[1] = readBuf[0];
				memcpy(pmbuf, headBuf, 2);
				pmbuf = pmbuf + 2;
				int data_length=0;
				nread(socket_low_id,&data_length, 4); //长度length
				memcpy(pmbuf, &data_length, 4); //长度length
				int dc_len = data_length + CHECK_LEN;  //数据长度+校验码长度
				int total_len = HEAD_LEN + 4 + dc_len;
				pmbuf = pmbuf +4;
				printf("data_length:%d,dc_len:%d",data_length,dc_len);
				//读剩下的字节(BLOCK_DATA数组 +check),因为数据比较长,建议1个字节循环读取	
				for(int i =0;i<dc_len;i++){
					nread(socket_low_id,&mapbBuf[i], 1);
				}
				memcpy(pmbuf, &mapbBuf[0], dc_len); //数据+校验
				// for(int i =0;i<total_len;i++){
				// 	LOG_INFO("map_recv:%x",mapBuf[i]);
				// }								
				U8 pack_code1 = mapBuf[total_len-2];	//取前字节
				U8 pack_code2 = mapBuf[total_len-1];	//取后字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(mapBuf,total_len-2);
				printf("map_checkcode:%d,pack_code:%d,low:%d,hig:%d",
						checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ //包正确,则取MAP数据
					map_receive_finish = false;
					int skip= sizeof(BLOCK_DATA);
					for(int i =0;i<data_length/skip;i++){	//BLOCK_DATA
						BLOCK_DATA *block =(BLOCK_DATA*)&mapBuf[6+i*skip];
						if(block!=NULL){
							int poseX = block->point.x;
							int poseY = block->point.y;
							U8	type  = block->type;
							/*if(type == OBSTACLE)
							{
								zmode_maps_download(poseX,poseY,type,data_length/skip);
							}*/
						}
					}
					map_receive_finish = true;	

				}else{
					printf("map_packet_check_error:%d,%d",pack_code,checkcode);
				}
				memset(mapBuf,0,sizeof(mapBuf));//清空缓存
				continue;								
			}
			else{
				printf("map_head_err:%x",readBuf[0]);
			}
		}	
		
		//分区数据
		if (readBuf[0] == POINT_DATA_HEAD_CHAR1) { //分区头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == POINT_DATA_HEAD_CHAR2) { //分区头2
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				//读剩下的字节(ROBOT_POSE+Point+check)
				//nread(socket_low_id,&contBuf[0], ZONE_PACKAGE_LEN-2);
				for(int i =0;i<ZONE_PACKAGE_LEN-2;i++){
					nread(socket_low_id,&contBuf[i], 1);
				}
				memcpy(pbuf, &contBuf[0], ZONE_PACKAGE_LEN-2);
				printf("zone_PACKAGE:%d \n",ZONE_PACKAGE_LEN);
				for(int i =0;i<ZONE_PACKAGE_LEN;i++){
					printf("zone_recv:%x \n",msgBuf[i]);
				}
				printf("zone_PACKAGE:%d \n",ZONE_PACKAGE_LEN);								
				U8 pack_code1 = msgBuf[ZONE_PACKAGE_LEN-2];   //取低字节
				U8 pack_code2 = msgBuf[ZONE_PACKAGE_LEN-1]; //取高字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,ZONE_PACKAGE_LEN-2);
				printf("zone_checkcode:%d,pack_code:%d,low:%d,hig:%d",checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ //包正确,则取ROBOT_POSE和Rect[2]数据
					ZONE_POSE *pose =(ZONE_POSE*)&msgBuf[HEAD_LEN];
					if(pose!=NULL){
						printf("pose_1");
						int poseX = pose->point.x;
						printf("pose_2");
						int poseY = pose->point.y;
						printf("pose_3");
						double yaw= pose->yaw;
						printf("pose_4");
						Point *point0 =(Point*)&msgBuf[HEAD_LEN+ZONE_LEN];
						// LOG_INFO("pose_5");
						// int tmpx =	point0->x;
						printf("pose_6");
						Point *point1 =(Point*)&msgBuf[HEAD_LEN+ZONE_LEN+POINT_LEN];
						printf("pose_7");
						//if(point0->x == 0 && point0->y == 0 && point1->x == 0 && point1->y == 0) {
							//clean_finish();
						//} 
						//else {
						followwall_zone_download(poseX,poseY,yaw,point0->x,point0->y,point1->x,point1->y);
							
						//}
					}else{
						printf("pose null");
					}
				}else{
					printf("zone_packet_check_error:%d,%d",pack_code,checkcode);
				}
				continue;								
			}
			else{
				printf("zone_head_err:%x",readBuf[0]);
			}
		}

		//直线角度
		if (readBuf[0] == LINE_HEAD_CHAR1) { //分区头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == LINE_HEAD_CHAR2) { //分区头2
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				//读剩下的字节
				//nread(socket_low_id,&contBuf[0], LINE_PACKAGE_LEN-2);
				for(int i =0;i<LINE_PACKAGE_LEN-2;i++){
					nread(socket_low_id,&contBuf[i], 1);
				}				
				memcpy(pbuf, &contBuf[0], LINE_PACKAGE_LEN-2); 
				for(int i =0;i<LINE_PACKAGE_LEN;i++){
					printf("line_recv:%x",msgBuf[i]);
				}								
				U8 pack_code1 = msgBuf[LINE_PACKAGE_LEN-2]; //取低字节
				U8 pack_code2 = msgBuf[LINE_PACKAGE_LEN-1]; //取高字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,LINE_PACKAGE_LEN-2);
				printf("line_checkcode:%d,pack_code:%d,low:%d,hig:%d",checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ 
					int *lineAngle =(int*)&msgBuf[HEAD_LEN];
					if(lineAngle!=NULL){
						printf("lineAngle:%d",*lineAngle);
						//line_angle_callback(*lineAngle);
					}
				}else{
					printf("line_packet_check_error:%d,%d",pack_code,checkcode);
				}
				continue;								
			}
			else{
				printf("line_head_err:%x",readBuf[0]);
			}
		}
		
		//指定点到点的路径
		if (readBuf[0] == PATH_HEAD_CHAR1) { //栅格头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == PATH_HEAD_CHAR2) { //栅格头2
				headBuf[1] = readBuf[0];
				memcpy(ppbuf, headBuf, 2);
				ppbuf = ppbuf + 2;
				int data_length=0;
				nread(socket_low_id,&data_length, 4); //长度length
				int path_count = data_length/8;
				memcpy(ppbuf, &data_length, 4); //长度length
				int dc_len = data_length + CHECK_LEN;  //数据长度+校验码长度
				int total_len = HEAD_LEN + 4 + dc_len;
				ppbuf = ppbuf +4;
				printf("data_length:%d,dc_len:%d",data_length,dc_len);
				//读剩下的字节(BLOCK_DATA数组 +check),因为数据比较长,建议1个字节循环读取	
				for(int i =0;i<dc_len;i++){
					nread(socket_low_id,&pathbBuf[i], 1);
				}
				memcpy(ppbuf, &pathbBuf[0], dc_len); //数据+校验
				// for(int i =0;i<total_len;i++){
				//  	LOG_INFO("path_recv:%x",pathBuf[i]);
				// }		
						
								
				U8 pack_code1 = pathBuf[total_len-2];	//取前字节
				U8 pack_code2 = pathBuf[total_len-1];	//取后字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(pathBuf,total_len-2);
				printf("path_checkcode:%d,pack_code:%d,low:%d,hig:%d",
						checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ //包正确,则取坐标数据
//					path_download(&pathBuf[6],data_length);
					Point path[path_count];
					//int number_point = path_count*2;
					int Astart_point[path_count][2] = {0};
					int j = 0;
					for(int i =0;i<data_length;i=i+4){
						int coord=0;
						memcpy(&coord, &pathBuf[6+i], 4);
						if(j%2 == 0){
							path[j/2].x = coord;
							Astart_point[j/2][0] = 	path[j/2].x;
						}else {
							path[j/2].y = coord;
							Astart_point[j/2][1] = 	path[j/2].y;	
							
						}
						
						j++;
						//LOG_INFO("path_download:%d,%d",i,coord);
					}
					path_download(path,path_count);
					chml::DpClient::Ptr netserver_client_ = NetserverInstruct::getNetInstruct()->GetDpnetserverClientPtr();
					
					std::string str_req, str_res;
					Json::Value json_req;
					json_req[JKEY_TYPE] = "astar_plan";
					//json_req[JKEY_BODY]["astar"][0][0] = Astart_point[number_point][2];
					for (int a_point = 0; a_point < path_count; a_point++)
					{
						json_req[JKEY_BODY]["astar"][a_point][0] = Astart_point[a_point][0];
						json_req[JKEY_BODY]["astar"][a_point][1] = Astart_point[a_point][1];
					}
					
					Json2String(json_req, str_req);
					netserver_client_->SendDpMessage(DP_SCHEDULE_BROADCAST, 0, str_req.c_str(), str_req.size(), &str_res);
				}
				else{
					printf("map_packet_check_error:%d,%d",pack_code,checkcode);
				}
				memset(mapBuf,0,sizeof(mapBuf));//清空缓存
				continue;								
			}
			else{
				printf("map_head_err:%x",readBuf[0]);
			}
		}		
		
		//控制命令cmd
		if (readBuf[0] == CMD_HEAD_CHAR1) { //命令cmd包头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == CMD_HEAD_CHAR2) { //命令cmd包头2
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				//读剩下的字节
				//nread(socket_low_id,&contBuf[0], LINE_PACKAGE_LEN-2);
				for(int i =0;i<CMD_PACKAGE_LEN-2;i++){
					nread(socket_low_id,&contBuf[i], 1);
				}				
				memcpy(pbuf, &contBuf[0], CMD_PACKAGE_LEN-2); 
				// for(int i =0;i<CMD_PACKAGE_LEN;i++){
				// 	LOG_INFO("cmd_recv:%x",msgBuf[i]);
				// }								
				U8 pack_code1 = msgBuf[CMD_PACKAGE_LEN-2]; //取低字节
				U8 pack_code2 = msgBuf[CMD_PACKAGE_LEN-1]; //取高字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,CMD_PACKAGE_LEN-2);
				//LOG_INFO("cmd_checkcode:%d,pack_code:%d,low:%d,hig:%d",checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ 
					U8 *cmd =(U8*)&msgBuf[HEAD_LEN];
					if(cmd!=NULL){
						cmd_publish(*cmd);
					}
				}else{
					printf("cmd_packet_check_error:%d,%d",pack_code,checkcode);
				}
				continue;								
			}
			else{
				printf("cmd_head_err:%x",readBuf[0]);
			}
		}
		
		if (readBuf[0] == DIST_HEAD_CHAR1) { //DIST头1
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == DIST_HEAD_CHAR2) { //DIST头2
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				//读剩下的字节
				for(int i =0;i<DIST_PACKAGE_LEN-2;i++){
					nread(socket_low_id,&contBuf[i], 1);
				}
				memcpy(pbuf, &contBuf[0], DIST_PACKAGE_LEN-2);								
				U8 pack_code1 = msgBuf[DIST_PACKAGE_LEN-2]; //取低字节
				U8 pack_code2 = msgBuf[DIST_PACKAGE_LEN-1]; //取高字节
				U16 pack_code = u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,DIST_PACKAGE_LEN-2);
				//LOG_INFO("dist_checkcode:%d,pack_code:%d,low:%d,hig:%d",checkcode,pack_code,pack_code1,pack_code2);
				if(pack_code == checkcode){ //包正确,则取ROBOT_POSE数据
					int *dist =(int*)&msgBuf[2];
					if(dist!=NULL){
						/*int now_state = get_now_state();								
						if(now_state == TOPMODE_FOLLOWWALL) 
						{
							wall_distance_download(*dist);
						}	
						else
						{
							recharge_distance_download(*dist);
						}*/
						//LOG_INFO("dist:%d",*dist);
					}								
				}else{
					printf("dist_packet_check_error:%d,%d",pack_code,checkcode);
				}				
				continue;									
			}
			else{
				printf("dist_head_err:%x",readBuf[0]);
			}
		}
		
		if(readBuf[0] == TOF_HEAD_CHAR1) {
			headBuf[0] = readBuf[0];
			nread(socket_low_id,readBuf,1);
			if (readBuf[0] == TOF_HEAD_CHAR2) { //命令cmd包头2
				headBuf[1] = readBuf[0];
				memcpy(pbuf, headBuf, 2);
				pbuf = pbuf + 2;
				//读剩下的字节
				for(int i =0;i<CMD_PACKAGE_LEN-2;i++){
					nread(socket_low_id,&contBuf[i], 1);
				}				
				memcpy(pbuf, &contBuf[0], CMD_PACKAGE_LEN-2); 							
				U8 pack_code1 = msgBuf[CMD_PACKAGE_LEN-2]; //取低字节
				U8 pack_code2 = msgBuf[CMD_PACKAGE_LEN-1]; //取高字节
				U16 pack_code =u8_to_u16(pack_code2,pack_code1);
				U16 checkcode = crc16check8(msgBuf,CMD_PACKAGE_LEN-2);
				if(pack_code == checkcode){ 
					U8 *cmd =(U8*)&msgBuf[HEAD_LEN];
					if(cmd!=NULL){
						//tof_msg_analyze(*cmd);
					}
				}else{
					printf("cmd_packet_check_error:%d,%d",pack_code,checkcode);
				}
				continue;								
			}
			else{
				printf("cmd_head_err:%x",readBuf[0]);
			}
		}
		//未定义的包
		else{
			if(!socket_low_conn){
				printf("socket_low_conn_closed_break!");
				break;
			}
			printf("low_client_error_package:%x",readBuf[0]);
			usleep(10000);
			continue;			
		}
	}
	return NULL;
}





pthread_mutex_t g_imu_lock;
int client_imu_send(char* buf,int len){
	pthread_mutex_lock(&g_imu_lock);
	int isend =write(socket_imu_id,buf,len);
	pthread_mutex_unlock(&g_imu_lock);
//	LOG_INFO("client_imu_send");
	return isend;
}
pthread_mutex_t g_odom_lock;
int client_odom_send(char* buf,int len){
	pthread_mutex_lock(&g_odom_lock);
	int isend =write(socket_odom_id,buf,len);
	pthread_mutex_unlock(&g_odom_lock);
//	LOG_INFO("client_odom_send");
	return isend;
}
pthread_mutex_t g_low_lock;
int client_low_send(char *buf,int len){
	pthread_mutex_lock(&g_low_lock);
	printf("write socket:%d",*buf);
	int isend =write(socket_low_id,buf,len);
	pthread_mutex_unlock(&g_low_lock);
	return isend;
}
pthread_mutex_t g_dist_lock;
int client_dist_send(char* buf,int len){
	pthread_mutex_lock(&g_dist_lock);
	int isend =write(socket_dist_id,buf,len);
	pthread_mutex_unlock(&g_dist_lock);
	return isend;
}



void navigate_init(){
	c_read_queue = (MSGQUEUE *)malloc(sizeof(MSGQUEUE));
	c_write_queue = (MSGQUEUE *)malloc(sizeof(MSGQUEUE));
	c_read_queue->queue_name  = "c_read_queue";
	c_write_queue->queue_name = "c_write_queue";
	init_queue(c_read_queue);
	init_queue(c_write_queue);
}

//void *navigateloop(void *agv){
void navigateloop(){	
	pthread_t thread[5];
	navigate_init();
	//IMU,ODOM,POSE,LOW-CONN
	if (pthread_create(&thread[0], NULL, check_connect, NULL) != 0)
	{
	}		
	//POSE-READ
	if (pthread_create(&thread[1], NULL, client_pose_read, NULL) != 0)
	{
	}	
	//LOW-READ
	if (pthread_create(&thread[2], NULL, client_low_read, NULL) != 0)
	{
	}

	while(1){
		if(socket_imu_conn && socket_odom_conn && socket_pose_conn && socket_low_conn ){
			printf("get_conn_to_server");
			system("echo 0 > /sys/class/leds/firefly:red:power/brightness");
			//api_play_voice(VOICE_POWER_ON);
			break;
		}	
		sleep(1);
	}
	
	pthread_join(thread[0], NULL);	
	pthread_join(thread[1], NULL);		
	pthread_join(thread[2], NULL);	
}

U16 u8_to_u16(U8 front,U8 behind){		//front:0xaa,behind:0xbb-->0xaabb
	U16 pack_code=0;
	pack_code= pack_code | front;
	pack_code = pack_code << 8;
	pack_code = pack_code| behind;
	return pack_code;
}

U16 crc16check(char *data, U32 len)
{
    U16 crc_gen = 0xa001;
    U16 crc;
    U32 i, j;
    /*init value of crc*/
    crc = 0xffff;
    if (len != 0)
    {
        for (i = 0; i < len; i++)
        {
            crc ^= (U16)(data[i]);
            for (j = 0; j < 8; j++)
            {
                if ((crc & 0x01) == 0x01)
                {
                    crc >>= 1;
                    crc ^= crc_gen;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
    } 
    return crc;
}
U16 crc16check8(U8 *data, U32 len)
{
    U16 crc_gen = 0xa001;
    U16 crc;
    U32 i, j;
    /*init value of crc*/
    crc = 0xffff;
    if (len != 0)
    {
        for (i = 0; i < len; i++)
        {
            crc ^= (U16)(data[i]);
            for (j = 0; j < 8; j++)
            {
                if ((crc & 0x01) == 0x01)
                {
                    crc >>= 1;
                    crc ^= crc_gen;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
    } 
    return crc;
}
//返回毫秒的时间戳
long getTimeStamp(){
	long mtime=0;
	struct timeval tv;
	gettimeofday(&tv, NULL);  //linux接口
	mtime = tv.tv_usec/1000 + tv.tv_sec*1000;  //us/1000锛宻*1000
	return mtime; // return ms
}
/*void pose_download(int x,int y,double angle){
	gAngle= (int)angle/10;
	coordinates->x = x;
	coordinates->y = y;
	printf("Point X=%d Point Y=%d gAngle=%d\n",coordinates->x,coordinates->y,gAngle);
}*/