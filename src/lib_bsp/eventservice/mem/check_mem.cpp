/**/ /*memory_trace.c*/
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "check_mem.h"

static void* my_malloc_hook(size_t size, const void* ptr);
static void my_free_hook(void* ptr, const void* caller);
static void* my_realloc_hook(void* ptr, size_t size, const void* caller);

static void* (*old_malloc_hook)(size_t size, const void* ptr);
static void (*old_free_hook)(void* ptr, const void* caller);
static void* (*old_realloc_hook)(void* ptr, size_t size, const void* caller);

#define BACK_TRACE_DEPTH 8
#define CACHE_SIZE       512

static FILE* g_memory_trace_fp = NULL;
static int g_memory_trace_cache_used = 0;
/**/ /*additional 3 items: alloc/free addr size*/
static void* g_memory_trace_cache[CACHE_SIZE][BACK_TRACE_DEPTH + 3];
static void  memory_trace_flush(void);
static void  memory_trace_write(int alloc, void* addr, int size);

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// declare
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define THREAD_AUTOLOCK automutex _lock_(&_mutex, __FUNCTION__, __LINE__);

class automutex {
public:
    automutex(pthread_mutex_t *mux, const char *func, const int line);
    ~automutex();

private:
    pthread_mutex_t *m_mux;
};

automutex::automutex(pthread_mutex_t *mux, const char *func, const int line) {
    m_mux = mux;
    pthread_mutex_lock(m_mux);
}

automutex::~automutex() {
    pthread_mutex_unlock(m_mux);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void memory_trace_backup(void) {
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  return;
}

static void memory_trace_hook(void) {
  __malloc_hook = my_malloc_hook;
  __free_hook = my_free_hook;
  __realloc_hook = my_realloc_hook;
  return;
}

static void memory_trace_restore(void) {
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;

  return;
}

void memory_trace_init(void) {
  THREAD_AUTOLOCK;

  if (g_memory_trace_fp == NULL) {
    char file_name[260] = {0};
    snprintf(file_name, sizeof(file_name), "./%d_memory.log", getpid());
    if ((g_memory_trace_fp = fopen(file_name, "wb+")) != NULL) {
      memory_trace_backup();
      memory_trace_hook();
    }

    atexit(memory_trace_deinit);
  }

  return;
}

void memory_trace_deinit(void) {
  THREAD_AUTOLOCK;

  if (g_memory_trace_fp != NULL) {
    memory_trace_restore();
    memory_trace_flush();
    fclose(g_memory_trace_fp);
    g_memory_trace_fp = NULL;
  }

  return;
}

// void (*__malloc_initialize_hook)(void) = memory_trace_init;

static void* my_malloc_hook(size_t size, const void* caller) {
  THREAD_AUTOLOCK;

  void* result = NULL;
  memory_trace_restore();
  result = malloc(size);
  memory_trace_write(1, result, size);
  memory_trace_hook();

  return result;
}

static void my_free_hook(void* ptr, const void* caller) {
  THREAD_AUTOLOCK;

  memory_trace_restore();
  free(ptr);
  memory_trace_write(0, ptr, 0);
  memory_trace_hook();

  return;
}

static void* my_realloc_hook(void* ptr, size_t size, const void* caller) {
  THREAD_AUTOLOCK;

  void* result = NULL;

  memory_trace_restore();
  memory_trace_write(0, ptr, 0);
  result = realloc(ptr, size);
  memory_trace_write(1, result, size);
  memory_trace_hook();

  return result;
}

static void memory_trace_flush_one_entry(int index) {
  int offset = 0;
  char buffer[512] = {0};
  int fd = fileno(g_memory_trace_fp);
  int alloc = (int)g_memory_trace_cache[index][BACK_TRACE_DEPTH];
  void* addr = g_memory_trace_cache[index][BACK_TRACE_DEPTH + 1];
  int size = (int)g_memory_trace_cache[index][BACK_TRACE_DEPTH + 2];
  void** backtrace_buffer = g_memory_trace_cache[index];

  snprintf(buffer, sizeof(buffer), "%s %p %d ", alloc ? "alloc" : "free", addr, size);
  if (!alloc) {
    write(fd, buffer, strlen(buffer));
    write(fd, "\n", 1);
    return;
  }

  char** symbols = backtrace_symbols(backtrace_buffer, BACK_TRACE_DEPTH);
  if (symbols != NULL) {
    int i = 0;
    offset = strlen(buffer);
    for (i = 0; i < BACK_TRACE_DEPTH; i++) {
      if (symbols[i] == NULL) {
        break;
      }
      char* begin = strchr(symbols[i], '(');
      if (begin != NULL) {
        *begin = ' ';
        char* end = strchr(begin, ')');
        if (end != NULL) {
          strcpy(end, " ");
        }
        strncpy(buffer + offset, begin, sizeof(buffer) - offset);
        offset += strlen(begin);
      }
    }
    write(fd, buffer, offset);
    free(symbols);
  }

  write(fd, "\n", 1);
  return;
}

static void memory_trace_flush(void) {
  int i = 0;
  for (i = 0; i < g_memory_trace_cache_used; i++) {
    memory_trace_flush_one_entry(i);
  }
  g_memory_trace_cache_used = 0;

  return;
}

static void memory_trace_write(int alloc, void* addr, int size) {
  if (g_memory_trace_cache_used >= CACHE_SIZE) {
    memory_trace_flush();
  }

  int i = 0;
  void* backtrace_buffer[BACK_TRACE_DEPTH] = {0};
  backtrace(backtrace_buffer, BACK_TRACE_DEPTH);

  for (i = 0; i < BACK_TRACE_DEPTH; i++) {
    g_memory_trace_cache[g_memory_trace_cache_used][i] = backtrace_buffer[i];
  }
  g_memory_trace_cache[g_memory_trace_cache_used][BACK_TRACE_DEPTH] = (void*)alloc;
  g_memory_trace_cache[g_memory_trace_cache_used][BACK_TRACE_DEPTH + 1] = addr;
  g_memory_trace_cache[g_memory_trace_cache_used][BACK_TRACE_DEPTH + 2] = (void*)size;

  g_memory_trace_cache_used++;

  return;
}

// #define MEMORY_TRACE_TEST
#ifdef MEMORY_TRACE_TEST
#include <string>
/* 定义线程pthread */
static void* pthread(void* arg) {
  char* p = (char*)malloc(105);
  p = (char*)malloc(128);

  free(p);
  return NULL;
}

void test(void) {
  char* p = (char*)malloc(100);
  p = (char*)malloc(123);

  free(p);

  return;
}

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = memory_trace_init;
int main(int argc, char* argv[]) {
  // memory_trace_init();

  // malloc(100);
  // test();
  // malloc(100);
  // test();
  // char* p = (char*)malloc(100);
  // free(p);

  // static char *pt = (char*)malloc(1924);

  /* 创建线程pthread */
  pthread_t tidp;
  if (pthread_create(&tidp, NULL, pthread, NULL) == -1) {
    printf("create error!\n");
    return 1;
  }

  pthread_join(tidp, NULL);

  // char *pn = new char[128];
  // delete[] pn;

  // std::string ss;
  // ss.resize(512);

  return 0;
}
#endif /*MEMORY_TRACE_TEST*/