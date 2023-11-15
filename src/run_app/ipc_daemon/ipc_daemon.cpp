
#include "ipc_daemon.h"
#include "app/app/app.h"
#include <linux/watchdog.h>
#include "wifi_switch.h"
#include "upgrade.h"
#include "eventservice/base/ping.h"

#define USE_WDT 0
#define SERVICE_CHECK_PERIOD_MS 3000
#define WDIOC_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT        _IOR(WATCHDOG_IOCTL_BASE, 7, int)
#define  TEST_HARD_VERSION  "1.0.0"

#define CONFIG_PATH_PREFIX "/userdata/"

#define DEVICE_NAME "扫地机器人"  //if Chinese character is used ,please make sure  econded as UTF-8
#define DEVICE_TYPE "167"
#define SOC_TYPE "RK3566"
#define DEVICE_MODEL "h3"

int print_flag = 1;
int online_flag = 0;
char g_acSn[50] = "D3A8004650DATET123451234" ; 
char g_softVer[15] = "0.0.1";
int upgrade_flag = 0;

int timeout1 = 22;
int timeout2;

static unsigned int g_server_check_ms=0;

extern void backtrace_init();

DP_MSG_HDR(IpcDaemon,DP_MSG_DEAL,pFUNC_DealDpMsg)

static DP_MSG_DEAL g_dpmsg_table[]={
  {"reset_factory",   &IpcDaemon::ResetFactory},
  {"ota_update",      &IpcDaemon::Ota_Upate}
};

static uint32 g_dpmsg_table_size = ARRAY_SIZE(g_dpmsg_table);

IpcDaemon::IpcDaemon()
  : AppInterface("ipc_daemon"){

}

IpcDaemon::~IpcDaemon(){
  #ifdef USE_WDT
  close(wdt_fd);
  #endif
}

bool IpcDaemon::PreInit(chml::EventService::Ptr event_service) {
  chml::SocketAddress dp_daemon_addr(ML_DP_SERVER_HOST, ML_DP_SERVER_PORT);
  dp_client=chml::DpClient::CreateDpClient(event_service,dp_daemon_addr,"dp_Daemon");
  ASSERT_RETURN_FAILURE(!dp_client, false);
  dp_client->SignalDpMessage.connect(this,&IpcDaemon::OnDpMessage);
  if (dp_client->ListenMessage("DP_DAEMON_REQUEST", chml::TYPE_REQUEST)) {
    DLOG_ERROR(MOD_EB,"dp client listen message failed.");
  }
  return true;
}

bool IpcDaemon::InitApp(chml::EventService::Ptr event_service) {
  g_server_check_ms=SERVICE_CHECK_PERIOD_MS;
  /*1.check db file*/  
  db_manager_init_db();
  /* 2. start services and check services status periodically*/
  daemon_services_init();
  /* 3. start dp services, process system command */
  daemon_services_start(g_server_check_ms);
  return true;
}

bool IpcDaemon::RunAPP(chml::EventService::Ptr event_service) {
  if (dp_client->ListenMessage("DP_DAEMON_REQUEST", chml::TYPE_REQUEST)) {
    DLOG_ERROR(MOD_EB,"dp client listen message failed.");
  }
  #ifdef USE_WDT
  OpenWdt();
  #endif

  m_ipcTh = new std::thread(std::bind(&IpcDaemon::Ipc_Monitor,this));
  m_otaTh = new std::thread(std::bind(&IpcDaemon::u_upgrade,this));
  return true;
}
void IpcDaemon::OnExitApp(chml::EventService::Ptr event_service) {
  #ifdef USE_WDT
  close(wdt_fd);
  #endif
}

int main(int argc,char *argv[]) {
  // GMainLoop *main_loop;
  printf("ipc_daemon app compile time: %s %s.\n", __DATE__, __TIME__);
  backtrace_init();
  bool bret = Log_ClientInit();
  //设置打印等级
  app::App::DebugPrint(argc, argv);  
  //设置当前主线程的名字
  chml::EventService::Ptr evt_srv = 
   chml::EventService::CreateCurrentEventService("ipc_daemon");
  ASSERT_RETURN_FAILURE((NULL == evt_srv), -2);
  app::AppInterface::Ptr app(new IpcDaemon());
  ASSERT_RETURN_FAILURE((NULL == app), -3);

  bret = app->PreInit(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -4);

  bret = app->InitApp(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -5);

  bret = app->RunAPP(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -6);

  app::App::AppRunFinish();  
  // main_loop = g_main_loop_new(NULL, FALSE);

  // g_main_loop_run(main_loop);
  // g_main_loop_unref(main_loop);
  while (true) {
    evt_srv->Run();
  }
  
  app->OnExitApp(evt_srv);

  return 0;  
}

void IpcDaemon::OnDpMessage(chml::DpClient::Ptr dp_cli, chml::DpMessage::Ptr dp_msg)
{
  int ret=-1;
  std::string stype;
  Json::Value jreq,jret;
  DLOG_INFO(MOD_EB,"=== dp %s",dp_msg->req_buffer.c_str());
  if(dp_msg->method=="DP_DAEMON_REQUEST")
  {
    if (!String2Json(dp_msg->req_buffer, jreq)) {
        DLOG_ERROR(MOD_EB,"json parse failed.");
        return;
    }
    stype = jreq[JKEY_TYPE].asString();
    DLOG_DEBUG(MOD_EB,"type==== %s",stype.c_str());
    for (uint32 idx = 0; idx<g_dpmsg_table_size; ++idx) {
        if (stype == g_dpmsg_table[idx].type) {
        ret = (this->*g_dpmsg_table[idx].handler) (jreq, jret);
        break;
        }
    }
    if(dp_msg->type == chml::TYPE_REQUEST) {
        jret[JKEY_TYPE]=stype;
        if(ret==0) {
          jret[JKEY_STATUS]=200;
          jret[JKEY_ERR]="success";
        }
        else {
          jret[JKEY_STATUS]=400;
          jret[JKEY_ERR]="error";
        }
        Json2String(jret, *dp_msg->res_buffer);
        DLOG_DEBUG(MOD_EB,"ret %s", (*dp_msg->res_buffer).c_str());
        dp_cli->SendDpReply(dp_msg);
    }
  }
}

void IpcDaemon::OpenWdt()
{
  wdt_fd=open("/dev/watchdog", O_WRONLY); //start watchdog
  if (wdt_fd == -1) {
        perror("watchdog");
        exit(EXIT_FAILURE);
  }
  ret = ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout1); //set timeout
  if (ret < 0)
      DLOG_ERROR(MOD_EB,"ioctl WDIOC_SETTIMEOUT failed.\n"); 

  ret = ioctl(wdt_fd, WDIOC_GETTIMEOUT, &timeout2); //get timeout
  if (ret < 0)
      DLOG_ERROR(MOD_EB,"ioctl WDIOC_SETTIMEOUT failed.\n"); 
  DLOG_DEBUG(MOD_EB,"timeout = %d\n", timeout2);

}

void IpcDaemon::Ipc_Monitor()
{
  while(1)
  {
    daemon_services_start(g_server_check_ms);
    #ifdef USE_WDT
    ret = write(wdt_fd, "\0", 1); //feed the dog
    if (ret != 1) {
        ret = -1;
        break;
    }
    #endif
    usleep(3000*1000);
  }
}

int IpcDaemon::ResetFactory(Json::Value &jreq, Json::Value &jret) {

  DLOG_DEBUG(MOD_EB,"&&&&&&&&reset factory&&&&&&&&\n");
  factory_reset();
  return 0;
}

int IpcDaemon::Ota_Upate(Json::Value &jreq, Json::Value &jret) {
  int ret=0;
  DLOG_DEBUG(MOD_EB,"&&&&&&&&Ota_Upate&&&&&&&&\n");
  ret=system_upgrade();
  return ret;
}

void IpcDaemon::u_upgrade()
{
	upgrade_para_t upgrade_para = {0};
	while(!chml::Ping::ping((char*)"wlan0",(char*)"14.215.177.38"))
	{
		sleep(5);
	}
	if(print_flag) printf("start service for upgrading\n");

	/**
	 *set @upgrade_para to customize for specific use.
	*/	
	strncpy(upgrade_para.sn, g_acSn, sizeof(g_acSn));
	strncpy(upgrade_para.software_version, g_softVer,sizeof(g_softVer));
	strcpy(upgrade_para.soc_type, SOC_TYPE);

	strcpy(upgrade_para.device_model, DEVICE_MODEL);
		
	strcpy(upgrade_para.hardware_version, TEST_HARD_VERSION);
	strcpy(upgrade_para.device_name, DEVICE_NAME);
	strcpy(upgrade_para.device_type, DEVICE_TYPE);
	strcpy(upgrade_para.path_prefix, CONFIG_PATH_PREFIX);
	upgrade_para.web_ui_enable=u_false;
	if (config_upgrade_para(&upgrade_para,upgrade_para.path_prefix) == u_false){
		return ;
	}

	int download_repeated_times=0;
	int polling_time;

	if(print_flag) printf("%s[%d]\n", __func__, __LINE__);
	if(upgrade_flag){
		respons_upgrade_result_to_cloude_server(response_upgraded_success);	//upgrade_flag升级后检测版本号标志，上报升级成功，版本号管理自行实现。
		upgrade_flag = 0;
	}

	char hardware_version[VERSION_BUF_SIZE] = {0};
	char software_version[VERSION_BUF_SIZE] = {0};

	while(1){	
		sleep(1);
		if(is_downloading()==u_false){
			if(have_new_version(software_version, hardware_version)==u_true){
				polling_time=get_polling_time();
				if(download_repeated_times<=get_check_times()){
					if((download())==u_true){
						if(get_upgrade_type()==upgrade_force){
							if(print_flag) printf("upgrade force at once!\n");
							respons_upgrade_result_to_cloude_server(response_starting_upgrade);
							download_repeated_times = 0;
							if(print_flag) printf("---------------------------------------------\n");
							if(print_flag) printf("----------------start update-----------------\n");
							if(print_flag) printf("---------------------------------------------\n");
							
							system("touch /userdata/update.cfg");       //用于标记升级,升级成功重启后进行版本号检测并设置 upgrade_flag
							
              upgrade_flag=system_upgrade();
							//system("recoverySystem update /userdata/update.img"); //升级固件
							while(1){
								sleep(5);
							}
						}else{
							if(print_flag) printf("downloaded upgrade file, waiting command for upgrade\n");
							//should never run here
						}
					}
					else{
						if(print_flag) printf("download upgrade package failed %d times\n", download_repeated_times); 
						system("rm /userdata/update.img;sync");//升级失败删除本地文件
						download_repeated_times++;
						sleep(1);
					}
				}
				else{
						if(print_flag) printf("etag check error for %d times, start to waiting %d seconds and then try again\n", download_repeated_times, polling_time);
						if(print_flag) printf("response download error status to server\n");
						respons_upgrade_result_to_cloude_server(response_pachage_check_erro); //don't care about response status
						download_repeated_times = 0;		
						break;
				}

			}
			else{
				if(print_flag) printf("no remote version available, wait 24H checking for the next time\n");
				break ;
			}
			
		}
		else{
			polling_time=get_polling_time();
			if(print_flag) printf("downloading, wait %d seconds checking for the next time checking new version\n", polling_time);
			sleep(polling_time);
		}
	}

}

