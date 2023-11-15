#ifndef _CHECK_MEM_H_
#define _CHECK_MEM_H_

#include <stdio.h>
#include <malloc.h>
#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

  void memory_trace_init(void);
  void memory_trace_deinit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif//_CHECK_MEM_H_
