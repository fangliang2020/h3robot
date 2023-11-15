#ifndef SHMC_GLOBAL_OPTIONS_H_
#define SHMC_GLOBAL_OPTIONS_H_

#include <functional>

namespace shmc {

/* Log level definition of the library
 */
enum LogLevel {
  kError = 1,
  kWarning,
  kInfo,
  kDebug,
};

/* Set the library log level and log handler
 * @log_level  - only logs with level <= @log_level will be handled
 * @log_func   - callback to output logs
 *
 * This function must be called before any container initialized.
 */
void SetLogHandler(LogLevel log_level, std::function<void(LogLevel, const char*)> log_func);

/* bit-flag of deciding when to create shm
 */
enum ShmCreateFlag {
  // (all bits cleared) never create shm only attach existing one
  kNoCreate = 0x0,
  // (1st bit) create shm if not exist
  kCreateIfNotExist = 0x1,
  // (2nd bit) create new shm if bigger size requested
  kCreateIfExtending = 0x2,
};

/* Set default create-flags used in container's InitForWrite
 * @create_flags  - bit-OR of some ShmCreateFlag
 *
 * By default kCreateIfNotExist is used when InitForWrite is running. You can
 * override it by calling this function. For example, if you are upgrading
 * the container for larger volume you can set kCreateIfExtending to allow
 * delete-then-recreate behavior automatically.
 * This function must be called before any container initialized.
 */
void SetDefaultCreateFlags(int create_flags);

}  // namespace shmc

#endif  // SHMC_GLOBAL_OPTIONS_H_
