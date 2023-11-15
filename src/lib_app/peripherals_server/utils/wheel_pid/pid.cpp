#include "pid.h"

#define Desire_left 100

MotorPid rMotorPid;
MotorPid lMotorPid;
MotorPid fMotorPid;
MotorPid mMotorPid;

#define MAX_OUT_PWM 65535
// #define MIN_PWM 21000 //启动基值

// #define MIN_PWM_L 23000 //启动基值


#define MIN_PWM 5000 //启动基值

#define MIN_PWM_L 5000 //启动基值

//增量式模糊PID参数
//PidParameter pidValue[] = {
//  {2,       2.0f,     30.0f,  	   1.0f,  2000 },
//  {5,       2.0f,     20.0f,      1.0f,  2000 },
//  {10,      2.0f,     15.0f,    	 1.0f,  2000 },
//  {15,      2.0f,     12.0f,  	   1.0f,  2000 },
//  {100,     2.5f,     25.0f,     1.0f,   1500 },
//};

//PidParameter pidValue[] = {
//  {2,        1.5f,     10.0f,  	   0.1f,  2000 },
//  {5,        1.7f,     13.0f,      0.5f,  2000 },
//  {10,       2.1f,     17.0f,    	 1.0f,  2000 },
//  {15,       2.3f,     20.0f,  	   1.0f,  2000 },
//  {100,      2.5f,     25.0f,     1.0f,   1500 },
////差值分段     KP        KI         KD     积分限幅
//};
float kp=0.0f,ki=0.0f,kd=0.0f;
//位置式
PidParameter pidValue[4];
PidParameter pidValue_last[] = {
  {5,        200.0f,     30.5f,          20.0f,   200 },
  {10,       300.0f,     40.0f,          20.0f,   500 },
  {50,       400.2f,     45.4f,    		 20.2f,   1500 },
  {100,      500.2f,     50.4f,    	 	 20.2f,   2000 },
//   {20,       250.8f,     20.6f,    	 50.2f,   13500 },
//   {30,       300.1f,     30.2f,    	 50.2f,   14000 },
//   {40,      350.6f,      30.8f,  	 50.4f,   17000 },
//   {60,      160.8f,      40.2f,      10.4f,   17000 },
//差值分段     KP        KI         KD     积分限幅
};

//初始化函数
int Bsp_PidInit(void)
{
	rMotorPid.pidParameter = pidValue;
	rMotorPid.currentError = 0;
	rMotorPid.lastError = 0;
	rMotorPid.sumError = 0;
	rMotorPid.out = 0;
	
	lMotorPid.pidParameter = pidValue;
	lMotorPid.currentError = 0;
	lMotorPid.lastError = 0;
	lMotorPid.sumError = 0;
	lMotorPid.out = 0;
	return 0;
}

int MotorPidclear(MotorPid *Mpid)
{
	memset(Mpid,0,sizeof(MotorPid));
	return 0;
}

/**********增量式左轮PID测试程序**********/
int PidFun_left(Motol_Ctrl *Motor_Item)
{
	int16_t absError;
	lMotorPid.Desire=abs(Motor_Item->setspeed);
	lMotorPid.Measure=abs(Motor_Item->readspeed);
	//absError=abs(lMotorPid.Desire);
	absError=abs(lMotorPid.Desire-lMotorPid.Measure);
	PidParameter* pidParameter = NULL;
	//根据期望速度匹配参数
	for(unsigned int i=0;i<(sizeof(pidValue)/sizeof(PidParameter));i++)
	{
		if(absError<pidValue[i].speed)
		{
			pidParameter=(PidParameter*)(&pidValue[i]);  //将参数传入
			lMotorPid.pidParameter=pidParameter;  //方便调试看
			break;
		}
	}
	if(pidParameter == NULL){
		return -1;
	}
	//PID运算
	lMotorPid.currentError = lMotorPid.Desire - lMotorPid.Measure;
	lMotorPid.sumError += lMotorPid.currentError;
    lMotorPid.out = (int)(pidParameter->kp*(lMotorPid.currentError-lMotorPid.lastError)+pidParameter->ki*lMotorPid.currentError\
		+ pidParameter->kd*(lMotorPid.currentError-2*lMotorPid.lastError+lMotorPid.preError));
	lMotorPid.preError = lMotorPid.lastError;
	lMotorPid.lastError = lMotorPid.currentError;
	lMotorPid.Sumout=lMotorPid.Sumout+lMotorPid.out;
//	printf("currentError is %d,Sumerr is %d\n",lMotorPid.currentError,lMotorPid.sumError);
//	if(lMotorPid.sumError >pidParameter->iLimit){
//		lMotorPid.sumError = pidParameter->iLimit;
//	}
//	if(lMotorPid.sumError < - pidParameter->iLimit){
//		lMotorPid.sumError = -pidParameter->iLimit;
//	}	
	
	if(lMotorPid.Sumout>MAX_OUT_PWM) lMotorPid.Sumout=MAX_OUT_PWM;
	else if(lMotorPid.Sumout<MIN_PWM_L)  lMotorPid.Sumout=MIN_PWM_L;  //添加启动时最小的基值

//	printf("lMotorPid.Sumout %d\n",lMotorPid.Sumout);
	return abs(lMotorPid.Sumout);	
}



/**********位置式左轮PID测试程序**********/
// int PidFun_left(Motol_Ctrl *Motor_Item)
// {
// 	int16_t absError;
// 	lMotorPid.Desire=Motor_Item->setspeed;
// 	lMotorPid.Measure=Motor_Item->readspeed;
// 	absError=abs(rMotorPid.Desire);
// 	//absError=abs(lMotorPid.Desire-lMotorPid.Measure);
// 	PidParameter* pidParameter = NULL;
// 	//根据期望速度匹配参数
// 	for(int i=0;i<(sizeof(pidValue)/sizeof(PidParameter));i++)
// 	{
// 		if(absError<pidValue[i].speed)
// 		{
// 			pidParameter=(PidParameter*)(&pidValue[i]);  //将参数传入
// 			lMotorPid.pidParameter=pidParameter;  //方便调试看
// 			break;
// 		}
// 	}
// 	if(pidParameter == NULL){
// 		return -1;
// 		printf("pidParameter is NULL\n");
// 	}
	
// 	//PID运算
// 	lMotorPid.currentError = lMotorPid.Desire - lMotorPid.Measure;
	
// 	lMotorPid.sumError += lMotorPid.currentError;
//     lMotorPid.out = (int)(pidParameter->kp*lMotorPid.currentError+pidParameter->ki*lMotorPid.sumError + pidParameter->kd*(lMotorPid.currentError - lMotorPid.lastError));
// 	lMotorPid.lastError = lMotorPid.currentError;
// 	//printf("currentError is %d,sumError is %d\n",lMotorPid.currentError,lMotorPid.sumError);
// 	//积分限幅
// 	if(lMotorPid.sumError >pidParameter->iLimit){
// 		lMotorPid.sumError = pidParameter->iLimit;
// 	}
// 	if(lMotorPid.sumError < - pidParameter->iLimit){
// 		lMotorPid.sumError = -pidParameter->iLimit;
// 	}	
// //	if(MOTOR_SPD.L_SPD.DIR==1)  //正向
// //	{
// //		if(lMotorPid.out>MAX_OUT_PWM) lMotorPid.out=MAX_OUT_PWM;
// //		else if(lMotorPid.out<MIN_PWM)  lMotorPid.out=MIN_PWM;
// //	}
// //	else if(MOTOR_SPD.L_SPD.DIR==0)  //反向
// //	{
// //		if(lMotorPid.out<-MAX_OUT_PWM) lMotorPid.out=-MAX_OUT_PWM;
// //		else if(lMotorPid.out>-MIN_PWM)  lMotorPid.out=-MIN_PWM;
// //	}

// 	if(abs(lMotorPid.out)>MAX_OUT_PWM)  lMotorPid.out=MAX_OUT_PWM;
// 	else if(abs(lMotorPid.out)<MIN_PWM)  lMotorPid.out=MIN_PWM;
// 	//printf("lMotorPid.out is %d\n",lMotorPid.out);
// 		return abs(lMotorPid.out); 
	
// }

int PidFun_right(Motol_Ctrl *Motor_Item)
{
	int16_t absError;
	rMotorPid.Desire=abs(Motor_Item->setspeed);
	rMotorPid.Measure=abs(Motor_Item->readspeed);
	//absError=abs(rMotorPid.Desire);
	absError=abs(rMotorPid.Desire-rMotorPid.Measure);
	PidParameter* pidParameter = NULL;
	//根据期望速度匹配参数
	for(unsigned int i=0;i<(sizeof(pidValue)/sizeof(PidParameter));i++)
	{
		if(absError<pidValue[i].speed)
		{
			pidParameter=(PidParameter*)(&pidValue[i]);  //将参数传入
			rMotorPid.pidParameter=pidParameter;  //方便调试看
			break;
		}
	}
	if(pidParameter == NULL){
		return -1;
	}
	//PID运算
	rMotorPid.currentError = rMotorPid.Desire - rMotorPid.Measure;
	rMotorPid.sumError += rMotorPid.currentError;
    rMotorPid.out = (int)(pidParameter->kp*(rMotorPid.currentError-rMotorPid.lastError)+pidParameter->ki*rMotorPid.currentError\
		+ pidParameter->kd*(rMotorPid.currentError-2*rMotorPid.lastError+rMotorPid.preError));
	rMotorPid.preError = rMotorPid.lastError;
	rMotorPid.lastError = rMotorPid.currentError;
	rMotorPid.Sumout=rMotorPid.Sumout+rMotorPid.out;
	//printf("currentError is %d,Sumerr is %d\n",rMotorPid.currentError,rMotorPid.sumError);
//	if(rMotorPid.sumError >pidParameter->iLimit){
//		rMotorPid.sumError = pidParameter->iLimit;
//	}
//	if(rMotorPid.sumError < - pidParameter->iLimit){
//		rMotorPid.sumError = -pidParameter->iLimit;
//	}	
	
	if(rMotorPid.Sumout>MAX_OUT_PWM) rMotorPid.Sumout=MAX_OUT_PWM;
	else if(rMotorPid.Sumout<MIN_PWM)  rMotorPid.Sumout=MIN_PWM;  //添加启动时最小的基值

	//printf("rMotorPid.Sumout %d\n",rMotorPid.Sumout);
	return abs(rMotorPid.Sumout);
	
}
// int PidFun_right(Motol_Ctrl *Motor_Item)
// {
// 	int16_t absError;
// 	rMotorPid.Desire=Motor_Item->setspeed;
// 	rMotorPid.Measure=Motor_Item->readspeed;
// 	//absError=abs(rMotorPid.Desire);
// 	absError=abs(rMotorPid.Desire-rMotorPid.Measure);
// 	PidParameter* pidParameter = NULL;
// 	//根据期望速度匹配参数
// 	for(int i=0;i<(sizeof(pidValue)/sizeof(PidParameter));i++)
// 	{
// 		if(absError<pidValue[i].speed)
// 		{
// 			pidParameter=(PidParameter*)(&pidValue[i]);  //将参数传入
// 			rMotorPid.pidParameter=pidParameter;  //方便调试看
// 			break;
// 		}
// 	}
// 	if(pidParameter == NULL){
// 		return -1;
// 	}
// 	//PID在积分项达到饱和时,误差仍然会在积分作用下继续累积，一旦误差开始反向变化，系统需要一定时间从饱和区退出，所以在u(k)达到最大和最小时，要停止积分作用，并且要有积分限幅和输出限幅.(转载)
// 	rMotorPid.currentError = rMotorPid.Desire - rMotorPid.Measure;
// 	rMotorPid.sumError += rMotorPid.currentError;
//     rMotorPid.out = (int)(pidParameter->kp*rMotorPid.currentError+pidParameter->ki*rMotorPid.sumError + pidParameter->kd*(rMotorPid.currentError - rMotorPid.lastError));
// 	rMotorPid.lastError = rMotorPid.currentError;
// 	printf("currentError is %d,sumError is %d\n",rMotorPid.currentError,rMotorPid.sumError);
// 	//积分限幅
// 	if(rMotorPid.sumError > pidParameter->iLimit){
// 		rMotorPid.sumError = pidParameter->iLimit;
// 	}
// 	if(rMotorPid.sumError < -(pidParameter->iLimit)){
// 		rMotorPid.sumError = -(pidParameter->iLimit);
// 	}	
// 	if(abs(rMotorPid.out)>MAX_OUT_PWM)  rMotorPid.out=MAX_OUT_PWM;
// 	else if(abs(rMotorPid.out)<MIN_PWM)  rMotorPid.out=MIN_PWM;
// 	printf("rMotorPid.out is %d\n",rMotorPid.out);
// 	return abs(rMotorPid.out); 
// 	//PID运算
// 	// rMotorPid.currentError = rMotorPid.Desire - rMotorPid.Measure;
// 	// rMotorPid.sumError += rMotorPid.currentError;
//     // rMotorPid.out = (int)(pidParameter->kp*rMotorPid.currentError+pidParameter->ki*rMotorPid.sumError + pidParameter->kd*(rMotorPid.currentError - rMotorPid.lastError));
// 	// rMotorPid.lastError = rMotorPid.currentError;
// 	// printf("currentError is %d,sumError is %d\n",rMotorPid.currentError,rMotorPid.sumError);
// 	// //积分限幅
// 	// if(rMotorPid.sumError > pidParameter->iLimit){
// 	// 	rMotorPid.sumError = pidParameter->iLimit;
// 	// }
// 	// if(rMotorPid.sumError < -(pidParameter->iLimit)){
// 	// 	rMotorPid.sumError = -(pidParameter->iLimit);
// 	// }	
// 	// if(abs(rMotorPid.out)>MAX_OUT_PWM)  rMotorPid.out=MAX_OUT_PWM;
// 	// else if(abs(rMotorPid.out)<MIN_PWM)  rMotorPid.out=MIN_PWM;
// 	// printf("rMotorPid.out is %d\n",rMotorPid.out);
// 	// return abs(rMotorPid.out); 
// }



//float fKp=100.0f;
// float fKi=30.0f;
// float fKd=20.0f;
float fKp=1390.0f;
float fKi=0.01f;
float fKd=0.09f;
uint16_t fLIMIT=2000;
int PidFun_Fan(Motol_Ctrl *Motor_Item)
{
	fMotorPid.Desire=Motor_Item->setspeed;
	fMotorPid.Measure=Motor_Item->readspeed;
	//PID运算
	fMotorPid.currentError = fMotorPid.Desire-fMotorPid.Measure;
    fMotorPid.out = (int)(fKp*(fMotorPid.currentError-fMotorPid.lastError)+fKi*fMotorPid.currentError\
		+ fKd*(fMotorPid.currentError-2*fMotorPid.lastError+fMotorPid.preError));
	fMotorPid.preError = fMotorPid.lastError;
	fMotorPid.lastError = fMotorPid.currentError;
	fMotorPid.Sumout=fMotorPid.Sumout+fMotorPid.out;
	std::cout <<"fMotorPid.Sumout is " << fMotorPid.Sumout<< std::endl;
	//积分限幅
	if(fMotorPid.Sumout>65000)
	{
		fMotorPid.Sumout=65000;
	}
	
	else if(fMotorPid.Sumout<10000)
	{
		fMotorPid.Sumout=10000;
	}
	return fMotorPid.Sumout; 	
}

float mKp=25.0f;
float mKi=7.0f;
float mKd=2.0f;
int PidFun_Mop(Motol_Ctrl *Motor_Item)
{
	mMotorPid.Desire=Motor_Item->setspeed;
	mMotorPid.Measure=Motor_Item->readspeed;
	//PID运算
	mMotorPid.currentError = mMotorPid.Desire-mMotorPid.Measure;
    mMotorPid.out = (int)(mKp*(mMotorPid.currentError-mMotorPid.lastError)+mKi*mMotorPid.currentError\
		+ mKd*(mMotorPid.currentError-2*mMotorPid.lastError+mMotorPid.preError));
	mMotorPid.preError = mMotorPid.lastError;
	mMotorPid.lastError = mMotorPid.currentError;
	mMotorPid.Sumout=mMotorPid.Sumout+mMotorPid.out;
	//积分限幅
	if(mMotorPid.Sumout>40000)
	{
		mMotorPid.Sumout=40000;
	}
	else if(mMotorPid.Sumout<9000)
	{
		mMotorPid.Sumout=9000;
	}
	return mMotorPid.Sumout; 	
}

