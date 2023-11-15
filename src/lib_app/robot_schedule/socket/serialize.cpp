#include "serialize.h"


//验证通过
void serialize_socket_map_data(SOCKET_MAP_DATA *data,char *out,int *len){
	char *p = out;
	char head[2]={0},check[2]={0},length[4]={0};int hlen=0,clen=0,llen=0;
	serialize_short(data->head,head,&hlen);	
	serialize_int(data->length,length,&llen);
	memcpy(p,head,hlen);
	p+=hlen;
	memcpy(p,length,llen);
	p+=llen;	
	int blockcnt = data->length / sizeof(BLOCK_DATA);
	//LOG_INFO("blockcnt:%d",blockcnt);
	for(int i =0;i<blockcnt;i++){
		char tmp[9]={0};int tmp_len=0;
		serialize_block_data(&data->data[i],tmp,&tmp_len);
		memcpy(p,tmp,tmp_len);
		p+=tmp_len;	
	}
	serialize_short(data->check,check,&clen);
	memcpy(p,check,clen);
	*len = hlen + llen + blockcnt*sizeof(BLOCK_DATA) + clen;
}
//验证通过
void serialize_socket_pose_data(SOCKET_POSE_DATA *data,char *out,int *len){
	char *p = out;
	char head[2]={0},check[2]={0},pp[20]={0};int hlen=0,clen=0,plen=0;
	serialize_short(data->head,head,&hlen);
	serialize_short(data->check,check,&clen);	
	serialize_pose_data(&data->data,pp,&plen);
	memcpy(p,head,hlen);
	p+=hlen;
	memcpy(p,pp,plen);
	p+=plen;
	memcpy(p,check,clen);
	*len = hlen + clen + plen;
}
//验证通过
void serialize_socket_zone_data(SOCKET_ZONE_DATA *data,char *out,int *len){
	char *p = out;
	char head[2]={0},check[2]={0},initPose[20]={0},rect0[20]={0},rect1[20]={0};
	int hlen=0,clen=0,ilen=0,r0len=0,r1len=0;
	serialize_short(data->head,head,&hlen);
	serialize_short(data->check,check,&clen);	
	serialize_pose_data(&data->initPose,initPose,&ilen);
	serialize_point_data(&data->rect[0],rect0,&r0len);
	serialize_point_data(&data->rect[1],rect1,&r1len);
	memcpy(p,head,hlen);
	p+=hlen;
	memcpy(p,initPose,ilen);
	p+=ilen;
	memcpy(p,rect0,r0len);
	p+=r0len;
	memcpy(p,rect1,r1len);
	p+=r1len;	
	memcpy(p,check,clen);
	*len = hlen + clen + ilen + r0len +r1len;	
}

//验证通过
void serialize_socket_imu_data(SOCKET_IMU_DATA *data,char *out,int *len){
	char *p = out;
	char head[2]={0},check[2]={0},imu[100]={0};int hlen=0,clen=0,ilen=0;
	serialize_short(data->head,head,&hlen);
	serialize_short(data->check,check,&clen);	
	serialize_imu_data(&data->data,imu,&ilen);
	memcpy(p,head,hlen);
	p+=hlen;
	memcpy(p,imu,ilen);
	p+=ilen;
	memcpy(p,check,clen);
	*len = hlen + clen + ilen;
}
//验证通过
void serialize_socket_odom_data(SOCKET_ODOM_DATA *data,char *out,int *len){
	char *p = out;
	char head[2]={0},check[2]={0},odom[100]={0};int hlen=0,clen=0,olen=0;
	serialize_short(data->head,head,&hlen);
	serialize_short(data->check,check,&clen);	
	serialize_odom_data(&data->data,odom,&olen);
	memcpy(p,head,hlen);
	p+=hlen;
	memcpy(p,odom,olen);
	p+=olen;
	memcpy(p,check,clen);
	*len = hlen + clen + olen;	
}



//低位放低字节(和小端模式一致),验证通过
void serialize_short(U16 data,char* out,int *len){
	char *p = out;		
	p[0] = data & 0xff; 		//低位放低字节
	p[1] = data >> 8 & 0xff;
	*len =2;
}
//低位放低字节(和小端模式一致),验证通过
void serialize_int(U32 data,char* out,int *len){
	char *p = out;
	p[0] = data & 0xff;				//低位放低字节
	p[1] = data >> 8 & 0xff; 
	p[2] = data >> 16 & 0xff;
	p[3] = data >> 24 & 0xff;
	*len =4;
	for(int i =0;i<*len;i++){
//		LOG_INFO("serialze_int:%x",out[i]);
	}	
}


//验证通过
void serialize_imu_data(IMU_DATA *data,char *out,int *len){
	char *p =out;
	int skip = sizeof(double);  //8
	memcpy(p,&data->vel_angle_x,skip);
	p +=skip;
	memcpy(p,&data->vel_angle_y,skip);
	p +=skip;
	memcpy(p,&data->vel_angle_z,skip);	
	p +=skip;
	memcpy(p,&data->acc_x,skip);
	p +=skip;
	memcpy(p,&data->acc_y,skip);
	p +=skip;
	memcpy(p,&data->acc_z,skip);
	*len = 6*skip;	
	for(int i =0;i<*len;i++){
//		LOG_INFO("serialize_imu_data:%x",out[i]);
	}
}
//验证通过
void serialize_odom_data(ODOM_DATA *data,char *out,int *len){
	char *p =out;
	int skip = sizeof(int);
	memcpy(p,&data->speed_left,skip);
	p +=skip;
	memcpy(p,&data->speed_right,skip);
	*len = 2* skip;
	for(int i =0;i<*len;i++){
//		LOG_INFO("serialize_odom_data:%x",out[i]);
	}	
}

//验证通过
void serialize_point_data(Point *data,char *out,int *len){
	char *p =out;
	int skip = sizeof(int);
	char outx[4]={0},outy[4]={0};int ix=0,iy=0;
	serialize_int(data->x,outx,&ix);
	serialize_int(data->y,outy,&iy);
	memcpy(p,outx,ix);
	p +=skip;
	memcpy(p,outy,iy);
	*len = 2* skip;

	for(int i =0;i<*len;i++){
//		LOG_INFO("serialize_point_data:%x",out[i]);
	}	


}

//验证通过
void serialize_block_data(BLOCK_DATA *data,char *out,int *len){
	char *p =out;
	char outp[20]={0};int intp=0;
	serialize_point_data(&data->point,outp,&intp);   //8
	memcpy(p,outp,intp);
	p+=intp;
	memcpy(p,&data->type,1);   //type
	*len = intp +1;
	for(int i =0;i<*len;i++){
//		LOG_INFO("serialize_block_data:%x",out[i]);
	}
}
void serialize_blocks_data(BLOCK_DATA *data,int blen,char *out,int *len){
	char *p = out;
	for(int i =0;i<blen;i++){
		char outx[20]={0};int outlen=0;
		serialize_block_data(&data[i],outx,&outlen);
		memcpy(p,outx,outlen);
		p+=outlen;
	}
	*len = blen* sizeof(BLOCK_DATA);
}

//验证通过
void serialize_pose_data(ROBOT_POSE *data,char *out,int *len){
	char *p =out;
	char outp[8]={0};int intp=0;
	serialize_point_data(&data->point,outp,&intp);   //point
	memcpy(p,outp,intp);
	p+=intp;
	memcpy(p,&data->yaw,sizeof(int));   //yaw
	*len = 12;
	for(int i =0;i<*len;i++){
//		LOG_INFO("serialize_pose_data:%x",out[i]);
	}
}

void serialize_zone_data(ROBOT_POSE *data0,Point *data1,char *out,int *len){
	char *p =out;
	char out0[50]={0},out1[50]={0},out2[50]={0};
	int ipose=0,irect1=0,irect2=0;
	serialize_pose_data(data0,out0,&ipose);
	serialize_point_data(&data1[0],out1,&irect1);
	serialize_point_data(&data1[1],out2,&irect2);
	memcpy(p,out0,ipose);
	p+=ipose;
	memcpy(p,out1,irect1);
	p+=irect1;
	memcpy(p,out2,irect2);
	*len = ipose + irect1 + irect2;
}

