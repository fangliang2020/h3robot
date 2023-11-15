/**
 * @Description: net server main
 * @Author:      zhz
 * @Version:     1
 */

#include "app/app/app.h"
#include "db_server/main/db_manager.h"

extern void backtrace_init();
int main(int argc, char *argv[]) {
  printf("db_server_app compile time: %s %s.\n", __DATE__, __TIME__);
  backtrace_init();
  bool bret = Log_ClientInit();

  app::App::DebugPrint(argc, argv);

  chml::EventService::Ptr evt_srv =
    chml::EventService::CreateCurrentEventService("db_server_app");
  ASSERT_RETURN_FAILURE((NULL == evt_srv), -2);

  app::AppInterface::Ptr app(new db::DbServer());
  ASSERT_RETURN_FAILURE((NULL == app), -3);

  bret = app->PreInit(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -4);

  bret = app->InitApp(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -5);

  bret = app->RunAPP(evt_srv);
  ASSERT_RETURN_FAILURE((false == bret), -6);

  app::App::AppRunFinish();

  while (true) {
    evt_srv->Run();
  }

  app->OnExitApp(evt_srv);

  return 0;
}