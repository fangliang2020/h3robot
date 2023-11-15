
#include <stdio.h>
#include "app/app/app.h"
#include "core_include.h"

#include "eventservice/util/mlgetopt.h"
#include "eventservice/vty/vty_client.h"

extern void backtrace_init();
int main(int argc, char *argv[]) {
  bool bret = false;
  printf("dp server compile %s %s\n", __DATE__, __TIME__);
  backtrace_init();

  bret = Log_ClientInit();
  ASSERT_RETURN_FAILURE((false == bret), -1);

  // set log config, need after log's init
  app::App::DebugPrint(argc, argv);

  int32_t ch = 0, arg_num;
  while ((ch = ml_getopt(argc, argv, (char*)"p:w:h")) != EOF) {
    switch (ch) {
    case 'p': {
      sscanf(ml_optarg, "%d", &arg_num);
      printf("%s[%d] log output level %d\n", __FUNCTION__, __LINE__, arg_num);
      Log_DbgAllSetLevel(arg_num);
      break;
    }

    case 'h':
      break;
    }
  }
  
  chml::EventService::Ptr ev_srv =
    chml::EventService::CreateCurrentEventService("dp_server");
  ASSERT_RETURN_FAILURE((NULL == ev_srv), -2);

  // dp server
  chml::DpServer::Ptr dp_server(new chml::DpServer(ev_srv));
  ASSERT_RETURN_FAILURE((NULL == dp_server), -3);

  const chml::SocketAddress dp_addr("0.0.0.0", ML_DP_SERVER_PORT);
  bret = dp_server->Start(dp_addr);
  ASSERT_RETURN_FAILURE((false == bret), -7);
  printf("[carl]:DpServer Running\n");

  app::App::AppRunFinish();

  while (true) {
    ev_srv->Run();
  }

  return 0;
}
