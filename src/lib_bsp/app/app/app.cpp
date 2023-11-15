#if defined(POSIX)
#endif  // defined POSIX
#if defined(WIN32)
#include <Windows.h>
#endif  // defined(WIN32)
#include <signal.h>

#include "app/app/app.h"
#include "eventservice/dp/dpserver.h"
#include "eventservice/util/mlgetopt.h"
#include "eventservice/util/proc_util.h"
#include "eventservice/log/log_client.h"

extern void backtrace_init();

namespace app {

#define ON_MSG_PRE_INIT 1
#define ON_MSG_INIT 2
#define ON_MSG_RUN_APP 3
#define ON_MSG_EXIT 4

static bool signal_exit;

struct AppInstance {
  AppInstance() {}
  ~AppInstance() {
    app.reset();
    es.reset();
  }
  AppInterface::Ptr app;
  chml::EventService::Ptr es;
};

struct AppData : public chml::MessageData {
  AppData() {}
  ~AppData() {
    app.reset();
    es.reset();
    signal_event.reset();
  }
  typedef boost::shared_ptr<AppData> Ptr;
  AppInterface::Ptr app;
  chml::EventService::Ptr es;
  chml::SignalEvent::Ptr signal_event;
};

class AppImpl : public App,
                public boost::noncopyable,
                public boost::enable_shared_from_this<AppImpl>,
                public sigslot::has_slots<>,
                public chml::MessageHandler {
 public:
  typedef boost::shared_ptr<AppImpl> Ptr;

 public:
  AppImpl() {
    // Make sure that ThreadManager is created on the main thread before
    // we start a new thread.
    printf(">>>>>>EventBase init start, Build time:%s, %s\n", __DATE__,  __TIME__);
    chml::ThreadManager::Instance();
    signal_event_ = chml::SignalEvent::CreateSignalEvent();
  }
  virtual ~AppImpl() {}

 public:
  bool InitPublicSystem(bool multi_proc_log) {
    main_es_ = chml::EventService::CreateCurrentEventService("ml_main_thread");
    return true;
  }

 public:
  virtual bool RegisterApp(AppInterface::Ptr app) {
    ASSERT_RETURN_FAILURE(!app, false);
    ASSERT_RETURN_FAILURE(!main_es_, false);
    AppInstance app_instance;
    app_instance.app = app;
    app_instance.es = chml::EventService::CreateEventService(
        NULL, app->app_name(), app->thread_stack_size());
    apps_.push_back(app_instance);
    return true;
  }

  virtual void AppRun(bool internal_blocking) {
    ASSERT_RETURN_VOID(!main_es_);
    // Run pre init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_PRE_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      DLOG_INFO(MOD_EB, "PreInit message Done:%s", iter->app->app_name().c_str());
    }
    // run init
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_INIT, app_data);
      signal_event_->WaitSignal(10 * 1000);
      DLOG_INFO(MOD_EB, "Init message Done: %s", iter->app->app_name().c_str());
    }
    // run app
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es = iter->es;
      app_data->signal_event = signal_event_;
      iter->es->Post(this, ON_MSG_RUN_APP, app_data);
      DLOG_INFO(MOD_EB, "Run message Done: %s", iter->app->app_name().c_str());
    }
    //
    if (internal_blocking) {
      main_es_->Run();
    }
  }

  void ExitSystem(void) {
    // ...
    Log_FlushCache();

    // sleep 3秒，等待Log线程记录日志
    chml::Thread::SleepMs(3 * 1000);

#if defined WIN32
    exit(127);
#elif defined UBUNTU64
    exit(127);
#elif defined LITEOS
    // LITEOS not support signal
#else
    exit(0);
#endif
  }

  virtual void ExitApp() {
    chml::SignalEvent::Ptr exit_signal = chml::SignalEvent::CreateSignalEvent();
    for (Apps::iterator iter = apps_.begin(); iter != apps_.end(); iter++) {
      iter->es = chml::EventService::CreateEventService();
      AppData::Ptr app_data(new AppData);
      app_data->app = iter->app;
      app_data->es = iter->es;
      app_data->signal_event = exit_signal;
      iter->es->Post(this, ON_MSG_EXIT, app_data);
      exit_signal->WaitSignal(100);
      DLOG_INFO(MOD_EB, "Exit Message Done: %s", iter->app->app_name().c_str());
    }
    DLOG_INFO(MOD_EB, "App exited:%d!!!", signal_exit);
    LOG_TRACE(LT_SYS, "%04d %s", LOG_ID_REBOOT_APP_EXIT, "APP EXIT Reboot Dev.");
    Log_FlushCache();
    usleep(500* 1000); //500ms
    if (!signal_exit) {
      // WIN32:exit app; Linux device:reboot system.
#ifdef DEVICE_SDK
    ML_DeviceSDK_Sys_Reboot();
#else
    system("reboot");
#endif
    } else {
      ExitSystem();
    }
  }

 public:
  virtual void OnMessage(chml::Message *msg) {
    AppData::Ptr app_data = boost::dynamic_pointer_cast<AppData>(msg->pdata);
    if (msg->message_id == ON_MSG_PRE_INIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_PRE_INIT");
      OnPreInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_INIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_INIT");
      OnInitApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_RUN_APP) {
      DLOG_INFO(MOD_EB, "ON_MSG_RUN_APP");
      OnRunApp(app_data->es, app_data->app, app_data->signal_event);
    } else if (msg->message_id == ON_MSG_EXIT) {
      DLOG_INFO(MOD_EB, "ON_MSG_EXIT");
      OnExitApp(app_data->es, app_data->app, app_data->signal_event);
    } else {
      DLOG_ERROR(MOD_EB, "Unkown message");
    }
  }

  void OnPreInitApp(chml::EventService::Ptr es, AppInterface::Ptr app,
                    chml::SignalEvent::Ptr signal_event) {
    if (!app->PreInit(es)) {
      DLOG_ERROR(MOD_EB, "App PreInit failed:%s", app->app_name().c_str());
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnInitApp(chml::EventService::Ptr es, AppInterface::Ptr app, chml::SignalEvent::Ptr signal_event) {
    if (!app->InitApp(es)) {
      DLOG_ERROR(MOD_EB, "App Init failed:%s", app->app_name().c_str());
      ExitApp();
    } else {
      signal_event->TriggerSignal();
    }
  }

  void OnRunApp(chml::EventService::Ptr es, AppInterface::Ptr app,
                chml::SignalEvent::Ptr signal_event) {
    if (!app->RunAPP(es)) {
      DLOG_ERROR(MOD_EB, "App Run failed:%s", app->app_name().c_str());
      ExitApp();
    }
  }

  void OnExitApp(chml::EventService::Ptr es, AppInterface::Ptr app,
                 chml::SignalEvent::Ptr signal_event) {
    app->OnExitApp(es);
    // 处理完毕，告诉App
    signal_event->TriggerSignal();
  }

 private:
  typedef std::list<AppInstance> Apps;
  Apps apps_;
  chml::CriticalSection crit_;
  chml::EventService::Ptr main_es_;
  chml::SignalEvent::Ptr signal_event_;
};

// 捕捉APP退出信号用
struct AppExit {
  AppImpl::Ptr appopt;
} appexit;

static void ExitSigHandler(int signumber) {
  DLOG_INFO(MOD_EB, "Signal %d captured, start to exit App!!!", signumber);
  signal_exit = true;
  appexit.appopt->ExitApp();
}

int ExitSigRegist(int signumber) {
#if defined(WIN32)
  if (SIG_ERR == signal(signumber, ExitSigHandler)) {
    DLOG_ERROR(MOD_EB, "the signal's catching faile!!");
    return -1;
  }
#elif defined(LITEOS)
  // ..
#else
  struct sigaction act, oldact;
  act.sa_handler = ExitSigHandler;
  act.sa_flags = 0;
  if (sigaction(signumber, &act, &oldact)) {
    DLOG_ERROR(MOD_EB, "the signal's catching faile!!");
    return -1;
  }
#endif
  printf("signal catcher test \n");
  return 0;
}

bool ExitSigCatch() {
#if defined(WIN32)
  if (-1 == ExitSigRegist(SIGBREAK)) {
    return false;
  }
#elif defined(LITEOS)
  // ..
#else
  // TODO backtrace_init();
  if (-1 == ExitSigRegist(SIGINT)) {
    return false;
  }
#endif
  return true;
}

App::Ptr App::CreateApp(bool multi_proc_log) {
  AppImpl::Ptr app(new AppImpl());
  // 开启app退出信号捕捉，POSIX下是捕捉Ctrl+C，WIN32下是捕捉Ctrl+Break
  appexit.appopt = app;
  signal_exit = false;
  if (!ExitSigCatch()) {
    DLOG_INFO(MOD_EB, "App Catch exit signal catch failed!");
  }
  if (app->InitPublicSystem(multi_proc_log)) {
    return app;
  }
  return App::Ptr();
}

void App::DebugPrint(int argc, char *argv[]) {
  int ch, level;
#ifdef USE_SYS_LOG
  sys_log_init();
  set_syslog_level(LOG_ERR);
#endif
  while ((ch = ml_getopt(argc, argv, (char *)"v:p:")) != EOF) {
    switch (ch) {
      case 'v':
        sscanf(ml_optarg, "%d", &level);
        printf("%s[%d] level %d\n", __FUNCTION__, __LINE__, level);
        // Log_SetSyncPrint(true);
        Log_DbgAllSetLevel(level);
        break;

      case 'p':
        sscanf(ml_optarg, "%d", &level);
        printf("%s[%d] level %d\n", __FUNCTION__, __LINE__, level);
        Log_SetSyncPrint(true);
        Log_DbgAllSetLevel(level);
#ifdef USE_SYS_LOG
        if (level == LL_DEBUG) {
          set_syslog_level(LOG_DEBUG);
        } else if (level == LL_INFO) {
          set_syslog_level(LOG_INFO);
        } else if (level == LL_WARNING) {
          set_syslog_level(LOG_WARNING);
        } else if (level == LL_ERROR) {
          set_syslog_level(LOG_ERR);
        } else if (level == LL_NONE) {
          set_syslog_level(-1);
        }
#endif
        break;
    }
  }
}

void App::AppRunFinish() {
#ifndef WIN32
  int ret = 0;
  char *fifo_path = NULL;
  int fd = -1;

  fifo_path = getenv("WRITE_FIFO_PATH");
  if (!fifo_path) {
    // printf("get env, WRITE_FIFO_PATH failed\n");
    return;
  }

  fd = open(fifo_path, O_WRONLY);
  if (fd < 0) {
    printf("open fifo failed, %s\n", strerror(errno));
    return;
  }

  std::string str = chml::XProcUtil::get_prog_short_name();
  str += ":";
  str += "RUN_FINISH";
  ret = write(fd, str.c_str(), str.length() + 1);
  if (ret < 0) {
    printf("write failed, %s\n", strerror(errno));
  }
  close(fd);
#endif
}

}  // namespace app
