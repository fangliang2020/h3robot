#ifndef EVENTSERVICES_UTIL_PROC_UTIL_H
#define EVENTSERVICES_UTIL_PROC_UTIL_H
#include <string>
#include <set>

namespace chml {

class XProcUtil {
 public:
  static bool check_process_running(const char* process_name);

  static int self_pid();
  static int self_tid();

  static const char *get_prog_name();
  static const char *get_prog_short_name();

  static int set_affinity(const std::set<int> &cpus);

  static bool set_sched_priority(int priority);
  static bool set_sched_priority_max();

#ifndef WIN32
  static int execute(std::string cmd, std::string& resultStr, int timeout = 5);
  static std::string execute(std::string cmd, int timeout = 5);
  //static std::vector<int> getPidsByName(const char* processName);
#endif
};

}  // namespace chml

#endif
