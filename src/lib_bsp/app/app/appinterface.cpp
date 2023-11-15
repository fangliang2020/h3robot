#include "eventservice/base/basictypes.h"
#include "eventservice/log/log_client.h"

#include "appinterface.h"

#ifdef ENABLE_BREAKPAD
#include "client/linux/handler/exception_handler.h"
#endif

extern void backtrace_init();

namespace app {
  
#ifdef ENABLE_BREAKPAD
static bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded) {
  printf("dump path: %s\n", descriptor.path());
  chmod(descriptor.path(), DEFFILEMODE);
  return succeeded;
}
#endif

AppInterface::AppInterface(const std::string app_name, size_t thread_stack_size)
    : app_name_(app_name), thread_stack_size_(thread_stack_size) {
#ifdef ENABLE_BREAKPAD
  static google_breakpad::MinidumpDescriptor descriptor("/mnt/log/system_server_files");
  static google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true, -1);
#endif
}

}  // namespace app
