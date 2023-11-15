#include "vl53l5cx_sensor.h"
#include "peripherals_server/main/device_manager.h"
std::vector<uint16_t> CommParam::vl53_dis(192,0);
namespace peripherals{
bool Vl53l5cxManager::need_stop_=false;
DP_MSG_HDR(Vl53l5cxManager,DP_MSG_DEAL,pFUNC_DealDpMsg)
static DP_MSG_DEAL g_dpmsg_table[]={
    {"tof_calibrate",   &Vl53l5cxManager::Vl53l5cx_Xtakcalib},  //tof校准，需要通过反光片
    {"tof_demarcate",   &Vl53l5cxManager::Vl53l5cx_offset},  //tof标定，需要放置障碍物
};
static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

Vl53l5cxManager::Vl53l5cxManager(PerServer *per_server_ptr) 
	: BaseModule(per_server_ptr) {
}
Vl53l5cxManager::~Vl53l5cxManager() {
  
}
int Vl53l5cxManager::Start() {
	int status;
	status = OpenVl53l5cxDev();
	if(status<0)
	{
		//报错处理
		DLOG_ERROR(MOD_EB, "open vl53l5cxdev error\r\n");
	}
	else if(status==0)
	{
		Vl53_right.name=(char*)"vl53_right";
		Vl53_mid.name=(char*)"Vl53_mid";
		Vl53_left.name=(char*)"Vl53_left";
		status=InitVl53l5cxDev(&Vl53_right);
		status=InitVl53l5cxDev(&Vl53_mid);
		status=InitVl53l5cxDev(&Vl53_left);
		if(status>=0) {
			tof_vector.resize(192);
			vl53_dis.reserve(192);
			Vl53l5cx_StartSample();
		}
	} 
	return 0;
}
int Vl53l5cxManager::Stop() {
  DeInitVl53l5cxDev();
	return 0;
}
int Vl53l5cxManager::OnDpMessage(std::string stype, Json::Value &jreq, Json::Value &jret) {
    DLOG_DEBUG(MOD_EB, "OnDpMessage:%s.", stype.c_str());
    int nret = -1;
    for (uint32 idx = 0; idx < g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
				need_stop_=true;
				tof_right->join();
				tof_mid->join();
				tof_left->join();
        nret = (this->*g_dpmsg_table[idx].handler) (jreq["body"], jret);
				need_stop_=false;
				Vl53l5cx_StartSample();
        break;
        }
    }

    return nret;   
}
int Vl53l5cxManager::OpenVl53l5cxDev()
{
	int status;
	status=vl53l5cx_comms_initial(&Vl53_right.platform,VL53RIGHT_PATH);
	if(status<0){
			vl53l5cx_comms_closeport(&Vl53_right.platform);
			DLOG_ERROR(MOD_EB, "VL53L5CX_RIGHT comms init failed\n");
			return -1;
	}
	status=vl53l5cx_comms_initial(&Vl53_mid.platform,VL53MID_PATH);
	if(status<0){
			vl53l5cx_comms_closeport(&Vl53_mid.platform);
			DLOG_ERROR(MOD_EB, "VL53L5CX_MID comms init failed\n");
			return -1;
	}
	status=vl53l5cx_comms_initial(&Vl53_left.platform,VL53LEFT_PATH);
	if(status<0){
			vl53l5cx_comms_closeport(&Vl53_left.platform);
			DLOG_ERROR(MOD_EB, "VL53L5CX_LEFT comms init failed\n");
			return -1;
	}
	return 0;
}
int Vl53l5cxManager::InitVl53l5cxDev(VL53L5CX_Configuration *p_dev)
{
	/*********************************/
	/*   VL53L5CX ranging variables  */
	/*********************************/

	uint8_t 				status,isAlive;
	uint32_t 				integration_time_ms;
	//VL53L5CX_ResultsData 	Results;		/* Results data from VL53L5CX */
	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L5CX sensor connected */
	status = vl53l5cx_is_alive(p_dev, &isAlive);
	if(!isAlive || status)
	{
		DLOG_ERROR(MOD_EB, "VL53L5CX not detected at requested address\n");
		return -1;
	}

	/* (Mandatory) Init VL53L5CX sensor */
	status = vl53l5cx_init(p_dev);
	if(status)
	{
		DLOG_ERROR(MOD_EB, "VL53L5CX ULD Loading failed\n");
		return -1;
	}
	DLOG_INFO(MOD_EB, "VL53L5CX ULD ready ! (Version : %s)\n",VL53L5CX_API_REVISION);
			

	/*********************************/
	/*        Set some params        */
	/*********************************/

	/* Set resolution in 8x8. WARNING : As others settings depend to this
	 * one, it must be the first to use.
	 */
	status = vl53l5cx_set_resolution(p_dev, VL53L5CX_RESOLUTION_8X8);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_set_resolution failed, status %u\n", status);
		return -1;
	}

	/* Set ranging frequency to 10Hz.
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l5cx_set_ranging_frequency_hz(p_dev, 10);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_set_ranging_frequency_hz failed, status %u\n", status);
		return -1;
	}

	/* Set target order to closest */
	status = vl53l5cx_set_target_order(p_dev, VL53L5CX_TARGET_ORDER_CLOSEST);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_set_target_order failed, status %u\n", status);
		return -1;
	}

	/* Get current integration time */
	status = vl53l5cx_get_integration_time_ms(p_dev, &integration_time_ms);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_get_integration_time_ms, status %u\n", status);
		return -1;
	}
	DLOG_INFO(MOD_EB,"Current integration time is : %d ms\n", integration_time_ms);
  return 0;
}
int Vl53l5cxManager::DeInitVl53l5cxDev()
{
    vl53l5cx_comms_closeport(&Vl53_right.platform);
    vl53l5cx_comms_closeport(&Vl53_mid.platform);
    vl53l5cx_comms_closeport(&Vl53_left.platform);
    return 0;
}
int Vl53l5cxManager::Vl53l5cx_Xtakcalib(Json::Value &jreq, Json::Value &jret)
{
/*********************************/
	/*   VL53L5CX ranging variables  */
	/*********************************/
  
	usleep(100);
	uint8_t 		status, loop, isAlive, isReady, i;
	std::string orient; 
	VL53L5CX_ResultsData 	Results;	/* Results data from VL53L5CX */
	uint8_t			xtalk_data[VL53L5CX_XTALK_BUFFER_SIZE];	/* Buffer containing Xtalk data */
	VL53L5CX_Configuration *p_dev;
	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/
  JV_TO_STRING(jreq,"orient",orient,"");
	if(0 == strcmp("right",orient.c_str()))
  {
		p_dev=&Vl53_right;
	}
	else if(0 == strcmp("mid",orient.c_str())) {
		p_dev=&Vl53_mid;
	}
	else if(0 == strcmp("left",orient.c_str())) {
		p_dev=&Vl53_left;
	}
	/* (Optional) Check if there is a VL53L5CX sensor connected */
	status = vl53l5cx_is_alive(p_dev, &isAlive);
	if(!isAlive || status)
	{
		DLOG_ERROR(MOD_EB,"VL53L5CX not detected at requested address\n");
		return status;
	}

	/* (Mandatory) Init VL53L5CX sensor */
	status = vl53l5cx_init(p_dev);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"VL53L5CX ULD Loading failed\n");
		return -1;
	}
	DLOG_INFO(MOD_EB,"VL53L5CX ULD ready ! (Version : %s)\n",VL53L5CX_API_REVISION);

			
	/*********************************/
	/*    Start Xtalk calibration    */
	/*********************************/

	/* Start Xtalk calibration with a 3% reflective target at 600mm for the
	 * sensor, using 4 samples.
	 */
	DLOG_INFO(MOD_EB,"Running Xtalk calibration...\n"); 
	 
	status = vl53l5cx_calibrate_xtalk(p_dev, 3, 4, 600);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_calibrate_xtalk failed, status %u\n", status);
		return -1;
	}else
	{
		DLOG_INFO(MOD_EB,"Xtalk calibration done\n");
		/* Get Xtalk calibration data, in order to use them later */
		status = vl53l5cx_get_caldata_xtalk(p_dev, xtalk_data);

		/* Set Xtalk calibration data */
		status = vl53l5cx_set_caldata_xtalk(p_dev, xtalk_data);
	}
	
	
	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l5cx_start_ranging(p_dev);

	loop = 0;
	while(loop < 10)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
 
		status = vl53l5cx_check_data_ready(p_dev, &isReady);

		if(isReady)
		{
			vl53l5cx_get_ranging_data(p_dev, &Results);

			/* As the sensor is set in 4x4 mode by default, we have a total 
			 * of 16 zones to print. For this example, only the data of first zone are 
			 * print */
			DLOG_DEBUG(MOD_EB,"Print data no : %3u\n", p_dev->streamcount);

			for(i = 0; i < 16; i++)
			{
				DLOG_DEBUG(MOD_EB,"Zone : %3d, Status : %3u, Distance : %4d mm\n",
					i,
					Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*i]);
			}
			DLOG_DEBUG(MOD_EB,"\n");
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		WaitMs(&p_dev->platform, 5);
	}

	status = vl53l5cx_stop_ranging(p_dev);
	DLOG_DEBUG(MOD_EB,"End of ULD demo\n");

	return 0;    
}
int Vl53l5cxManager::Vl53l5cx_offset(Json::Value &jreq, Json::Value &jret)
{
	return 0;
}
int Vl53l5cxManager::Vl53l5cx_VizualizeXtakcalib(VL53L5CX_Configuration *p_dev) //0-right 1-mid 2-left
{
	/*********************************/
	/*   VL53L5CX ranging variables  */
	/*********************************/

	uint8_t 		status, loop, isAlive, isReady;
	VL53L5CX_ResultsData 	Results;	/* Results data from VL53L5CX */
	uint8_t			xtalk_data[VL53L5CX_XTALK_BUFFER_SIZE];	/* Buffer containing Xtalk data */


	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L5CX sensor connected */
	status = vl53l5cx_is_alive(p_dev, &isAlive);
	if(!isAlive || status)
	{
		DLOG_DEBUG(MOD_EB,"VL53L5CX not detected at requested address\n");
		return status;
	}

	/* (Mandatory) Init VL53L5CX sensor */
	status = vl53l5cx_init(p_dev);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"VL53L5CX ULD Loading failed\n");
		return status;
	}

	DLOG_INFO(MOD_EB,"VL53L5CX ULD ready ! (Version : %s)\n",
			VL53L5CX_API_REVISION);
			
			
	/*********************************/
	/*    Start Xtalk calibration    */
	/*********************************/

	/* Start Xtalk calibration with a 3% reflective target at 600mm for the
	 * sensor, using 4 samples.
	 */
	 
	DLOG_INFO(MOD_EB,"Running Xtalk calibration...\n");
	status = vl53l5cx_calibrate_xtalk(p_dev, 3, 4, 600);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_calibrate_xtalk failed, status %u\n", status);
		return status;
	}else
	{
		DLOG_INFO(MOD_EB,"Xtalk calibration done\n");
		
		/* Get Xtalk calibration data, in order to use them later */
		status = vl53l5cx_get_caldata_xtalk(p_dev, xtalk_data);

		/* Set Xtalk calibration data */
		status = vl53l5cx_set_caldata_xtalk(p_dev, xtalk_data);
	}
	
	/* (Optional) Visualize Xtalk grid and Xtalk shape */
	uint32_t i, j;
	union Block_header *bh_ptr;
	uint32_t xtalk_signal_kcps_grid[VL53L5CX_RESOLUTION_8X8];
	uint16_t xtalk_shape_bins[144];

	/* Swap buffer */
	SwapBuffer(xtalk_data, VL53L5CX_XTALK_BUFFER_SIZE);

	/* Get data */
	for(i = 0; i < VL53L5CX_XTALK_BUFFER_SIZE; i = i + 4)
	{
		bh_ptr = (union Block_header *)&(xtalk_data[i]);
		if (bh_ptr->idx == 0xA128){
			DLOG_INFO(MOD_EB,"Xtalk shape bins located at position %#06x\n", i);
			for (j = 0; j < 144; j++){
				memcpy(&(xtalk_shape_bins[j]), &(xtalk_data[i + 4 + j * 2]), 2);
				DLOG_INFO(MOD_EB,"xtalk_shape_bins[%d] = %u\n", j, xtalk_shape_bins[j]);
			}
		}
		if (bh_ptr->idx == 0x9FFC){
			DLOG_INFO(MOD_EB,"Xtalk signal kcps located at position %#06x\n", i);
			for (j = 0; j < VL53L5CX_RESOLUTION_8X8; j++){
				memcpy(&(xtalk_signal_kcps_grid[j]), &(xtalk_data[i + 4 + j * 4]), 4);
				xtalk_signal_kcps_grid[j] /= 2048;
				DLOG_INFO(MOD_EB,"xtalk_signal_kcps_grid[%d] = %d\n", j, xtalk_signal_kcps_grid[j]);
			}
		}
	}

	/* Re-Swap buffer (in case of re-using data later) */
	SwapBuffer(xtalk_data, VL53L5CX_XTALK_BUFFER_SIZE);

	
	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l5cx_start_ranging(p_dev);

	loop = 0;
	while(loop < 10)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
 
		status = vl53l5cx_check_data_ready(p_dev, &isReady);

		if(isReady)
		{
			vl53l5cx_get_ranging_data(p_dev, &Results);

			/* As the sensor is set in 4x4 mode by default, we have a total 
			 * of 16 zones to print. For this example, only the data of first zone are 
			 * print */
			DLOG_DEBUG(MOD_EB,"Print data no : %3u\n", p_dev->streamcount);
			for(i = 0; i < 16; i++)
			{
				DLOG_INFO(MOD_EB,"Zone : %3d, Status : %3u, Distance : %4d mm\n",
					i,
					Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*i]);
			}
			DLOG_INFO(MOD_EB,"\n");
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		WaitMs(&p_dev->platform, 5);
	}

	status = vl53l5cx_stop_ranging(p_dev);
	DLOG_INFO(MOD_EB,"End of ULD demo\n");
	return status;
}
int Vl53l5cxManager::RebootDev(VL53L5CX_Configuration *p_dev)
{
	/*********************************/
	/*   VL53L5CX ranging variables  */
	/*********************************/

	uint8_t 	status, isAlive;
	//VL53L5CX_ResultsData 	Results;		/* Results data from VL53L5CX */


	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L5CX sensor connected */
	status = vl53l5cx_is_alive(p_dev, &isAlive);
	if(!isAlive || status)
	{
		DLOG_ERROR(MOD_EB,"VL53L5CX not detected at requested address\n");
		return status;
	}

	/* (Mandatory) Init VL53L5CX sensor */
	status = vl53l5cx_init(p_dev);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"VL53L5CX ULD Loading failed\n");
		return status;
	}

	DLOG_INFO(MOD_EB,"VL53L5CX ULD ready ! (Version : %s)\n",
			VL53L5CX_API_REVISION);
			
	/*********************************/
	/*     Change the power mode     */
	/*********************************/

	/* For the example, we don't want to use the sensor during 10 seconds. In order to reduce
	 * the power consumption, the sensor is set to low power mode.
	 */
	status = vl53l5cx_set_power_mode(p_dev, VL53L5CX_POWER_MODE_SLEEP);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_set_power_mode failed, status %u\n", status);
		return status;
	}
	DLOG_INFO(MOD_EB,"VL53L5CX is now sleeping\n");

	/* We wait 5 seconds, only for the example */
	DLOG_INFO(MOD_EB,"Waiting 5 seconds for the example...\n");
	WaitMs(&p_dev->platform, 5000);

	/* After 5 seconds, the sensor needs to be restarted */
	status = vl53l5cx_set_power_mode(p_dev, VL53L5CX_POWER_MODE_WAKEUP);
	if(status)
	{
		DLOG_ERROR(MOD_EB,"vl53l5cx_set_power_mode failed, status %u\n", status);
		return status;
	}
	DLOG_INFO(MOD_EB,"VL53L5CX is now waking up\n");
    return 0;
}
void Vl53l5cxManager::Vl53l5cx_StartSample()
{
  tof_right = new std::thread(std::bind(&Vl53l5cxManager::RangingInterrupt,this,&Vl53_right));
	tof_mid= new std::thread(std::bind(&Vl53l5cxManager::RangingInterrupt,this,&Vl53_mid));
	tof_left= new std::thread(std::bind(&Vl53l5cxManager::RangingInterrupt,this,&Vl53_left));
	tof_right->detach();
	tof_mid->detach();
	tof_left->detach();
}
void Vl53l5cxManager::RangingInterrupt(VL53L5CX_Configuration *p_dev)
{
    uint8_t isReady,point;
    VL53L5CX_ResultsData 	Results;
    vl53l5cx_start_ranging(p_dev);
    while(1)
    {
      isReady = WaitForL5Interrupt(&p_dev->platform);
			if(isReady)
			{
				vl53l5cx_get_ranging_data(p_dev, &Results);
 
				/* As the sensor is set in 8x8 mode, we have a total
				* of 64 zones to print. For this example, only the data of
				* first zone are print */
				// printf("Print data no : %3u\n", p_dev->streamcount);
					for(point = 0; point < 64; point++)
					{
						if(0 == strcmp("vl53_right",p_dev->name))
						{
							tof_vector[point].x=12-(point/8);
							tof_vector[point].y=point%8;
							// if(Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==5
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==6
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==9) //达到置信度
							// {
							vl53_dis[point]=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
							tof_vector[point].dis=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
							// }
							// printf("Zone : %3d, x : %3d,y: %3d,  Distance : %4d mm\n",point,tof_vector[point].x,tof_vector[point].y,
							// tof_vector[point].dis);
						}
						else if(0 == strcmp("Vl53_mid",p_dev->name))
						{
							if(point<=31){
									tof_vector[point+64].x=4-(point/8);
							}  
							else{
									tof_vector[point+64].x=3-(point/8);
							}  
							tof_vector[point+64].y=point%8;
							//  if(Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==5
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==6
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==9) //达到置信度
							// {
								vl53_dis[point+64]=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
								tof_vector[point+64].dis=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
							// }
							// printf("Zone : %3d, x : %3d,y: %3d,  Distance : %4d mm\n",
		// point+64,
		// tof_vector[point+64].x,
		// tof_vector[point+64].y,
							// tof_vector[point+64].dis);
						}
						else if(0 == strcmp("Vl53_left",p_dev->name))
						{
							tof_vector[point+128].x=-(point/8)-5;
							tof_vector[point+128].y=point%8;
							// if(Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==5
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==6
							// ||Results.target_status[VL53L5CX_NB_TARGET_PER_ZONE*point]==9) //达到置信度
							// {
								vl53_dis[point+128]=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
								tof_vector[point+128].dis=Results.distance_mm[VL53L5CX_NB_TARGET_PER_ZONE*point];
							// }
							// printf("Zone : %3d, x : %3d,y: %3d,  Distance : %4d mm\n",
		// point+128,
		// tof_vector[point+128].x,
		// tof_vector[point+128].y,
							// tof_vector[point+128].dis);
						}      
					}
				// for(i = 0; i < 192; i++)
				// {
				// 	printf("Zone : %3d, x : %3u,y: %3u,  Distance : %4d mm\n",
				// 		i,
				// 		tof_vector[i].x,
				// 		tof_vector[i].y,
							//         tof_vector[i].dis);
				// }
				// printf("\n");

			}
			if(need_stop_){
				break;
			}
    }
}

}


