#ifndef SRC_BASE_BASICTYPES_H_
#define SRC_BASE_BASICTYPES_H_

#define ml_new new(std::nothrow)

#if !defined(__WINDOWS__) && !defined(__GNUC__)

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
#define __WINDOWS__   1
#elif defined(__linux__)
#define __GNUC__      1
#else
#error "Unrecognized OS platform"
#endif

#endif  // !defined(__WINDOWS__) && !defined(__GNUC__)

#if (defined(_DEBUG) || defined(DEBUG)) && !defined(__DEBUG__)
#define __DEBUG__         1    // debug version
#endif  // (defined(_DEBUG) || defined(DEBUG)) && !defined(__DEBUG__)

#if defined(_MSC_VER)
# pragma warning(disable:4996) // VC++ 2008 deprecated function warnings
# pragma warning(disable:4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
# pragma warning(disable:4355) // warning C4355: 'this': used in base member initializer list
# pragma warning(disable:4819) // warning C4819: The file contains a character that cannot be represented in the current code page (936)
#endif  // defined(_MSC_VER)

#ifdef __GNUC__
#    define GCC_VERSION_AT_LEAST(x,y) (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#    define GCC_VERSION_AT_LEAST(x,y) 0
#endif

#if GCC_VERSION_AT_LEAST(3,1)
#    define attribute_deprecated __attribute__((deprecated))
#elif defined(_MSC_VER)
#    define attribute_deprecated __declspec(deprecated)
#else
#    define attribute_deprecated
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <process.h>
#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>  // for uintptr_t
#endif
#elif defined(__GNUC__)
#include <unistd.h>
#include <stdint.h>  // int64_t, uint64_t
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <semaphore.h>
//#include <typeinfo>
//#include <iconv.h>
//#include <linux/types.h>
#else   /* __GNUC__ */
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif  /* __GNUC__ */

#include <stddef.h>  // for NULL, size_t

#define mlstd chml
// #define mlstd std

#if !defined(INT_TYPES_DEFINED)
#define INT_TYPES_DEFINED
#ifdef COMPILER_MSVC
typedef unsigned __int64  uint64;
typedef __int64           int64;
#ifndef INT64_C
#define INT64_C(x) x ##   I64
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ##  UI64
#endif
#define INT64_F           "I64"
#else  // COMPILER_MSVC
// On Mac OS X, cssmconfig.h defines uint64 as uint64_t
// TODO(fbarchard): Use long long for compatibility with chromium on BSD/OSX.
#if defined(OSX)
typedef uint64_t           uint64;
typedef int64_t            int64;
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "l"
#elif defined(__LP64__)
typedef unsigned long      uint64;  // NOLINT
typedef long               int64;  // NOLINT
#ifndef INT64_C
#define INT64_C(x) x ## L
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UL
#endif
#define INT64_F "l"
#else  // __LP64__
typedef unsigned long long uint64;  // NOLINT
typedef long long          int64;   // NOLINT
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "ll"
#endif  // __LP64__
#endif  // COMPILER_MSVC
typedef unsigned int       uint32;
typedef int                int32;
typedef unsigned short     uint16;  // NOLINT
typedef short              int16;   // NOLINT
typedef unsigned char      uint8;
typedef signed char        int8;
// typedef unsigned long      uint32;
#endif  // INT_TYPES_DEFINED

// Detect compiler is for x86 or x64.
#if defined(__x86_64__) || defined(_M_X64) || \
    defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif
// Detect compiler is for arm.
#if defined(__arm__) || defined(_M_ARM)
#define CPU_ARM 1
#endif
#if defined(CPU_X86) && defined(CPU_ARM)
#error CPU_X86 and CPU_ARM both defined.
#endif
#if !defined(ARCH_CPU_BIG_ENDIAN) && !defined(ARCH_CPU_LITTLE_ENDIAN)
// x86, arm or GCC provided __BYTE_ORDER__ macros
#if CPU_X86 || CPU_ARM ||  \
  (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ARCH_CPU_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ARCH_CPU_BIG_ENDIAN
#else
#error ARCH_CPU_BIG_ENDIAN or ARCH_CPU_LITTLE_ENDIAN should be defined.
#endif
#endif
#if defined(ARCH_CPU_BIG_ENDIAN) && defined(ARCH_CPU_LITTLE_ENDIAN)
#error ARCH_CPU_BIG_ENDIAN and ARCH_CPU_LITTLE_ENDIAN both defined.
#endif

#ifdef WIN32
typedef int socklen_t;
#endif

#define  ML_MIN(a,b) ((a) < (b) ? (a) : (b))
#define  ML_MAX(a,b) ((a) > (b) ? (a) : (b))

/* 四字节向上对齐,即大于等于x的能被4整除的最小的整数 */
#define ML_ALIGN32(x) \
  (((x) + sizeof(uint32) - 1) & (((uint32)-1) ^ (sizeof(uint32) - 1)))  // NOLINT

/* 八字节向上对齐,即大于等于x的能被4整除的最小的整数 */
#define ML_ALIGN64(x) \
  (((x) + sizeof(uint64) - 1) & (((uint64)-1) ^ (sizeof(uint64) - 1)))  // NOLINT

/* 按uint32, 向上对齐,即大于等于x的能被4或8整除的最小的整数 
#define ML_ALIGN_ULONG(x) \
  (((x) + sizeof(uint32) - 1) & (((uint32)-1) ^ (sizeof(uint32) - 1)))  // NOLINT
*/

// 通过type类型结构体的成员变量member的地址,获取该结构体的地址
#define ML_GET_STRUCT_HEAD(ptr, type, member) \
  ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))    // NOLINT

// 获取结构体中某个成员相对于该结构体首元素地址的偏移量
#define ML_GET_MEMBER_OFFSET(ptr, type, member) \
  (unsigned long)(&((type *)0)->member)                              // NOLINT

// The following only works for C++
#ifdef __cplusplus
namespace chml {
template<class T> inline T _min(T a, T b) {
  return (a > b) ? b : a;
}
template<class T> inline T _max(T a, T b) {
  return (a < b) ? b : a;
}

// For wait functions that take a number of milliseconds, kForever indicates
// unlimited time.
const int kForever = -1;
}  // namespace chml

#define ALIGNP(p, t) \
  (reinterpret_cast<uint8*>(((reinterpret_cast<uintptr_t>(p) + \
                              ((t) - 1)) & ~((t) - 1))))
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))

// Note: UNUSED is also defined in common.h
#ifndef UNUSED
#define UNUSED(x) Unused(static_cast<const void*>(&x))
#endif

#ifndef UNUSED2
#define UNUSED2(x, y) Unused(static_cast<const void*>(&x)); \
  Unused(static_cast<const void*>(&y))
#endif

#ifndef UNUSED3
#define UNUSED3(x, y, z) Unused(static_cast<const void*>(&x)); \
  Unused(static_cast<const void*>(&y)); \
  Unused(static_cast<const void*>(&z))
#endif

#ifndef UNUSED4
#define UNUSED4(x, y, z, a) Unused(static_cast<const void*>(&x)); \
  Unused(static_cast<const void*>(&y)); \
  Unused(static_cast<const void*>(&z)); \
  Unused(static_cast<const void*>(&a))
#endif

#ifndef UNUSED5
#define UNUSED5(x, y, z, a, b) Unused(static_cast<const void*>(&x)); \
  Unused(static_cast<const void*>(&y)); \
  Unused(static_cast<const void*>(&z)); \
  Unused(static_cast<const void*>(&a)); \
  Unused(static_cast<const void*>(&b))
#endif

inline void Unused(const void*) {}

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x) / sizeof(x[0])))
#endif  // ARRAY_SIZE

// Borrowed from Chromium's base/compiler_specific.h.
// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(WIN32)
#define OVERRIDE override
#elif defined(__clang__)
// Clang defaults to C++03 and warns about using override. Squelch that.
// Intentionally no push/pop here so all users of OVERRIDE ignore the warning
// too. This is like passing -Wno-c++11-extensions, except that GCC won't die
// (because it won't see this pragma).
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
// GCC 4.7 supports explicit virtual overrides when C++11 support is enabled.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  // WARN_UNUSED_RESULT

#if !defined(PACKED)
#if defined(__GNUC__)
#define PACKED __attribute__((__packed__))
#else
#define PACKED
#endif
#endif  // PACKED

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

inline bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression) {
  UNUSED4(function, file, line, expression);
  if (!result) {
    return false;
  }
  return true;
}

#ifndef ML_VERIFY
#define ML_VERIFY(x) Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif

// Use these to declare and define a static local variable (static T;) so that
// it is leaked so that its destructors are not called at exit.
#define MLSDK_DEFINE_STATIC_LOCAL(type, name, arguments) \
  static type& name = *new type arguments

#endif  // __cplusplus

#define FILE_PATH_SIZE  128

#endif  // SRC_BASE_BASICTYPES_H_
