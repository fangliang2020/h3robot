#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(WIN32)
#include <WS2tcpip.h>
#include <comdef.h>
#include <direct.h>
#include <winsock2.h>
#elif defined(POSIX)
#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eventservice/event/thread.h"
#include "eventservice/util/time_util.h"

#include "eventservice/log/log_cache.h"
#include "eventservice/log/log_client.h"
#include "eventservice/log/log_server.h"

/**
 * Log server status
 */
typedef struct {
  LOG_TIME_S last_time;
  uint16 write_overtime_cnt[LOG_SRV_FILE_TYPE_MAX]; /**< Write file wait times*/
  bool next_need_write[LOG_SRV_FILE_TYPE_MAX];
  bool flush_cache;
  uint8 cur_file_id[LOG_SRV_FILE_TYPE_MAX]; /**< Current write file index [0:1]*/
  uint32 cur_file_len[LOG_SRV_FILE_TYPE_MAX]; /**< Current file length */
  uint32 cache_len[LOG_SRV_FILE_TYPE_MAX];    /**< Current cache size*/
  uint8 *cache[LOG_SRV_FILE_TYPE_MAX];
  uint32 cur_log_id;
#if defined(WIN32)
  SOCKET udp_sockfd;
  sockaddr_in udp_target_sockaddr;
#else
  int udp_sockfd;
  struct sockaddr_in udp_target_sockaddr;
#ifdef LOG_PAUSE
  bool paused;
  int cmd_sockfd;
#endif
#endif
  char *output_buf;
} LOG_SRV_STATUS_S;

LOG_SRV_STATUS_S g_logsvr_status; /**< Log server status */

#ifdef LOG_PAUSE
#define LOG_SRV_CMD_PORT 6669
int Log_SrvRecvPack(uint32_t timeout);
#endif
/**
 * Log moduler
 */
typedef struct {
  /* service thread ID */
#ifdef POSIX
  pthread_t thread_;
#elif defined WIN32
  HANDLE thread_;
  DWORD thread_id_;
#endif
  bool stop_flag; /* Moduler destory flag */

  LOG_SRV_CONFIG_S config;
} LOG_SRV_ENTITY_S;

LOG_SRV_ENTITY_S *g_LogServer = NULL;
LOG_PRINT_END_E *LogPrintEnd;

LOGSVR_MUTEX_T g_log_srv_mutex;

bool Log_SrvGetFileType(LOG_TYPE_E type, LOG_SRV_FILE_TYPE_E *out_fid) {
  // 将DEBUG文件添加到最前面来
  if (type == LT_DEBUG) {
    *out_fid = LFT_DEBUG;
  } else if (type == LT_POINT) {
    *out_fid = LFT_POINT;
  } else if (type == LT_SYS) {
    *out_fid = LFT_SYS;
  } else if (type == LT_DEV) {
    *out_fid = LFT_DEV;
  } else if (type == LT_UI) {
    *out_fid = LFT_UI;
  } else if (type == LT_USER1) {
    *out_fid = LFT_USER1;
  } else if (type == LT_USER2) {
    *out_fid = LFT_USER2;
  } else if (type == LT_USER3) {
    *out_fid = LFT_USER3;
  } else {
    printf("[log]log server type error=%d!!!\n", type);
    *out_fid = LFT_SYS;
    return false;
  }
  return true;
}

bool Log_SrvGetType(LOG_SRV_FILE_TYPE_E fid, LOG_TYPE_E *out_type) {
  if (fid == LFT_POINT) {
    *out_type = LT_POINT;
  } else if (fid == LFT_SYS) {
    *out_type = LT_SYS;
  } else if (fid == LFT_DEV) {
    *out_type = LT_DEV;
  } else if (fid == LFT_UI) {
    *out_type = LT_UI;
  } else if (fid == LFT_USER1) {
    *out_type = LT_USER1;
  } else if (fid == LFT_USER2) {
    *out_type = LT_USER2;
  } else if (fid == LFT_USER3) {
    *out_type = LT_USER3;
  } else {
    printf("[log]log server fid error=%d!!!\n", fid);
    *out_type = LT_DEBUG;
    return false;
  }
  return true;
}

void Log_SrvGetFileName(LOG_SRV_FILE_TYPE_E fid, int index, char *out_data_name,
                        char *out_index_name, int maxlen) {
  const char *foldpath;
#if defined(WIN32)
  // 增加DEBUG日志文件
  const char *sformat_data[] = {"%s\\debug%d.log",  "%s\\point%d.log",
                                "%s\\sys%d.log",    "%s\\dev%d.log",
                                "%s\\ui%d.log",     "%s\\user1_%d.log",
                                "%s\\user2_%d.log", "%s\\user3_%d.log"};
  const char *sformat_index[] = {"%s\\debug%d_id.log",  "%s\\point%d_id.log",
                                 "%s\\sys%d_id.log",    "%s\\dev%d_id.log",
                                 "%s\\ui%d_id.log",     "%s\\user1_%d_id.log",
                                 "%s\\user2_%d_id.log", "%s\\user3_%d_id.log"};
#else
  const char *sformat_data[] = {"%s/debug%d.log",  "%s/point%d.log",
                                "%s/sys%d.log",    "%s/dev%d.log",
                                "%s/ui%d.log",     "%s/user1_%d.log",
                                "%s/user2_%d.log", "%s/user3_%d.log"};
  const char *sformat_index[] = {"%s/debug%d_id.log",  "%s/point%d_id.log",
                                 "%s/sys%d_id.log",    "%s/dev%d_id.log",
                                 "%s/ui%d_id.log",     "%s/user1_%d_id.log",
                                 "%s/user2_%d_id.log", "%s/user3_%d_id.log"};
#endif

  if (NULL != g_LogServer && g_LogServer->config.foldpath[0] != 0) {
    foldpath = g_LogServer->config.foldpath;
  } else {
    foldpath = kLogFoldPathDefault;
  }

  if (NULL == out_data_name || NULL == out_index_name) {
    printf("param is null.\n");
    return;
  }
  snprintf(out_data_name, maxlen, sformat_data[fid], foldpath, index);
  snprintf(out_index_name, maxlen, sformat_index[fid], foldpath, index);
}

void Log_SrvGetFileNameCurrent(LOG_SRV_FILE_TYPE_E fid, char *out_data_name,
                               char *out_index_name, int maxlen) {
  Log_SrvGetFileName(fid, g_logsvr_status.cur_file_id[fid], out_data_name,
                     out_index_name, maxlen);
}

bool Log_SrvFoldCreate(const char *fold_path) {
  int i;
  int len;
  char buf[128];

  if (fold_path == NULL || strlen(fold_path) == 0) {
    return false;
  }
  strncpy(buf, fold_path, 128);
  len = strlen(fold_path);

#ifdef WIN32
  for (i = 3; i < len; i++) {
    if (buf[i] == '\\') {
      buf[i] = '\0';
#ifndef __MINGW32__
      ::CreateDirectory(buf, NULL);
#else
      mkdir(buf);
#endif
      buf[i] = '\\';
    }
  }
#ifndef __MINGW32__
      ::CreateDirectory(buf, NULL);
#else
      mkdir(buf);
#endif
#else
  if (access(buf, 0) == 0) {
    return true;
  }
  for (i = 1; i < len; i++) {
    if (buf[i] == '/') {
      buf[i] = '\0';
      if (access(buf, 0) != 0) {
        mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
      }
      buf[i] = '/';
    }
  }
  if (len > 0 && access(buf, 0) != 0) {
    mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
  }
  if (access(buf, 0) != 0) {
    return false;
  }
#endif
  return true;
}

void Log_SrvConfigDefaultGet(LOG_SRV_CONFIG_S *config) {
  memset(config, 0, sizeof(LOG_SRV_CONFIG_S));
  snprintf(config->foldpath, LOG_FOlDPATH_MAXLEN, kLogFoldPathDefault);

  // 设置默认文件大小
  config->file_maxlen[LFT_DEBUG] = 512 * 1024;
  config->file_maxlen[LFT_POINT] = 5 * 1024 * 1024;
  config->file_maxlen[LFT_SYS] = 64 * 1024;
  config->file_maxlen[LFT_DEV] = 128 * 1024;
  config->file_maxlen[LFT_UI] = 128 * 1024;
  config->file_maxlen[LFT_USER1] = 128 * 1024;
  config->file_maxlen[LFT_USER2] = 128 * 1024;
  config->file_maxlen[LFT_USER3] = 128 * 1024;

  config->cache_maxsize[LFT_DEBUG] = 10 * 1024;
  config->cache_maxsize[LFT_POINT] = 10 * 1024;
  config->cache_maxsize[LFT_SYS] = 3 * 1024;
  config->cache_maxsize[LFT_DEV] = 3 * 1024;
  config->cache_maxsize[LFT_UI] = 3 * 1024;
  config->cache_maxsize[LFT_USER1] = 3 * 1024;
  config->cache_maxsize[LFT_USER2] = 3 * 1024;
  config->cache_maxsize[LFT_USER3] = 3 * 1024;

  config->pull_wait_ms = 10;

  config->write_overtime_cnt[LFT_DEBUG] = 6000;
  config->write_overtime_cnt[LFT_POINT] = 6000;
  config->write_overtime_cnt[LFT_SYS] = 10;
  config->write_overtime_cnt[LFT_DEV] = 6000;
  config->write_overtime_cnt[LFT_UI] = 6000;
  config->write_overtime_cnt[LFT_USER1] = 6000;
  config->write_overtime_cnt[LFT_USER2] = 6000;
  config->write_overtime_cnt[LFT_USER3] = 6000;
  // 默认设置DEBUG打印到本地
  config->print_end[LFT_DEBUG] = LE_LOCAL;
  // 默认设置其他类型写到文件里面去
  for (int i = LFT_POINT; i < LOG_SRV_FILE_TYPE_MAX; i++) {
    config->print_end[i] = LE_FILE;
  }
}

void Log_SrvConfigGet(LOG_SRV_CONFIG_S *config) {
  if (g_LogServer != NULL) {
    memcpy(config, &g_LogServer->config, sizeof(LOG_SRV_CONFIG_S));
  } else {
    Log_SrvConfigDefaultGet(config);
  }
}

bool Log_SvrIsGoodFileData(FILE *data_file) {
  uint8 buf[sizeof(LOG_NODE_FILE_S) + 2];
  size_t rdlen;
  LOG_NODE_FILE_S *p = (LOG_NODE_FILE_S *)buf;
  uint16 tail_length;
  int rslt;

  fseek(data_file, 0, SEEK_SET);
  rdlen = fread(buf, 1, 7, data_file);
  if (rdlen == 0) {
    return true;
  }
  if (rdlen < 7 || p->head != LOG_FILE_NODE_HEAD ||
      p->node.length < sizeof(LOG_NODE_S)) {
    return false;
  }
  rslt = fseek(data_file, p->node.length - 2, SEEK_CUR);
  if (rslt != 0) {
    return false;
  }
  rdlen = fread(&tail_length, 2, 1, data_file);
  if (rdlen != 1) {
    return false;
  }
  if (tail_length != p->node.length) {
    return false;
  }
  return true;
}

bool Log_SvrIsGoodFileIndex(FILE *data_file, FILE *index_file) {
  uint8 head;
  uint32 data_addr = 0;
  size_t rdlen;
  int rslt;

  fseek(data_file, 0, SEEK_SET);
  fseek(index_file, 0, SEEK_SET);

  for (;;) {
    rdlen = fread(&data_addr, 1, 4, index_file);
    if (rdlen == 0) {
      return true;
    }
    if (rdlen != 4) {
      return false;
    }
    rslt = fseek(data_file, data_addr, SEEK_SET);
    if (rslt != 0) {
      return false;
    }
    rdlen = fread(&head, 1, 1, data_file);
    if (rdlen != 1) {
      return false;
    }
    if (head != LOG_FILE_NODE_HEAD) {
      return false;
    }
  }
  return true;
}

void Log_SvrRebuildIndex(FILE *data_file, FILE *index_file) {
  uint8 buf[sizeof(LOG_NODE_FILE_S)];
  LOG_NODE_FILE_S *p = (LOG_NODE_FILE_S *)buf;
  int rslt;
  uint32 data_addr = 0;
  size_t rdlen;
  uint16 tail_length;

  fseek(data_file, 0, SEEK_SET);
  fseek(index_file, 0, SEEK_SET);

  for (;;) {
    data_addr = ftell(data_file);
    rdlen = fread(buf, 1, 7, data_file);
    if (rdlen < 7 || p->head != LOG_FILE_NODE_HEAD ||
        p->node.length < sizeof(LOG_NODE_S)) {
      break;
    }
    rslt = fseek(data_file, p->node.length - 2, SEEK_CUR);
    if (rslt != 0) {
      break;
    }
    rdlen = fread(&tail_length, 2, 1, data_file);
    if (rdlen != 1) {
      break;
    }
    if (tail_length != p->node.length) {
      break;
    }
    fwrite(&data_addr, 4, 1, index_file);
  }
}

uint32 Log_SvrGetMaxId(const char *data_name, const char *index_name,
                       uint32 *file_len) {
  uint8 buf[sizeof(LOG_NODE_FILE_S)];
  LOG_NODE_FILE_S *p = (LOG_NODE_FILE_S *)buf;
  FILE *data_file;
  FILE *index_file;
  uint32 id = 0;
  uint32 data_addr = 0;
  int rslt;
  size_t rdlen;

  do {
    *file_len = 0;
    data_file = fopen(data_name, "rb");
    index_file = fopen(index_name, "rb");

    if (!data_file) break;
    if (!index_file) break;
    rslt = fseek(index_file, -4, SEEK_END);
    if (rslt != 0) break;
    fread(&data_addr, 4, 1, index_file);
    rslt = fseek(data_file, data_addr, SEEK_SET);
    if (rslt != 0) break;
    rdlen = fread(buf, 1, 7, data_file);
    if (rdlen < 7 || p->head != LOG_FILE_NODE_HEAD) {
      break;
    }
    id = p->id;
  } while (0);

  if (data_file != NULL) {
    fseek(data_file, 0, SEEK_END);
    *file_len = ftell(data_file);
    fclose(data_file);
  }
  if (index_file != NULL) {
    fclose(index_file);
  }
  return id;
}

uint32 Log_SvrCheckSingleFile(LOG_SRV_FILE_TYPE_E file_type, int subfile_index,
                              uint32 *file_len) {
  char data_name[LOG_FILFPATH_MAXLEN];
  char index_name[LOG_FILFPATH_MAXLEN];
  FILE *data_file;
  FILE *index_file;
  bool is_good_data = false;
  bool is_good_index = false;

  Log_SrvGetFileName(file_type, subfile_index, data_name, index_name,
                     LOG_FILFPATH_MAXLEN);

  do {
    data_file = fopen(data_name, "rb");
    index_file = fopen(index_name, "rb");

    if (!data_file) {
      is_good_data = false;
      is_good_index = false;
      break;
    }
    if (!index_file) {
      is_good_index = false;
      break;
    }
    is_good_data = Log_SvrIsGoodFileData(data_file);
    if (is_good_data) {
      is_good_index = Log_SvrIsGoodFileIndex(data_file, index_file);
    } else {
      is_good_index = false;
    }
  } while (0);

  if (!is_good_data) {  // delete data and index file
    if (data_file) {
      fclose(data_file);
      data_file = NULL;
    }
    if (index_file) {
      fclose(index_file);
      index_file = NULL;
    }
    data_file = fopen(data_name, "wb");
    if (data_file) {
      fclose(data_file);
      data_file = NULL;
    }
    index_file = fopen(index_name, "wb");
    if (index_file) {
      fclose(index_file);
      index_file = NULL;
    }
  } else if (!is_good_index) {  // rebuild index file
    if (index_file) {
      fclose(index_file);
      index_file = NULL;
    }
    index_file = fopen(index_name, "wb");
    if (index_file) {
      Log_SvrRebuildIndex(data_file, index_file);
    }
  }
  if (data_file) {
    fclose(data_file);
    data_file = NULL;
  }
  if (index_file) {
    fclose(index_file);
    data_file = NULL;
  }

  return Log_SvrGetMaxId(data_name, index_name, file_len);
}

void Log_SrvCheckFileIndex() {
  uint32 curid[2];
  uint32 curfilelen[2];
  int fid = 0;
  int idmax;

  g_logsvr_status.cur_log_id = 0;

  for (int itype = 0; itype < LOG_SRV_FILE_TYPE_MAX; itype++) {
    idmax = 0;
    for (int i = 0; i < LOG_FILF_GROUP_MAXCNT; i++) {
      curid[i] =
          Log_SvrCheckSingleFile((LOG_SRV_FILE_TYPE_E)itype, i, &curfilelen[i]);
      if (curid[i] > g_logsvr_status.cur_log_id) {
        g_logsvr_status.cur_log_id = curid[i];
      }
      if (curid[i] > idmax) {
        idmax = curid[i];
        fid = i;
      }
    }
    g_logsvr_status.cur_file_id[itype] = fid;
    g_logsvr_status.cur_file_len[itype] = curfilelen[fid];
  }
}

bool Log_SrvSetPrintEnd(LOG_TYPE_E type, LOG_PRINT_END_E print_end) {
  if (!g_LogServer) {
    printf("error:logSrv uninit, Log_SrvSetPrintEnd failed\n");
    return false;
  }
  LOG_SRV_FILE_TYPE_E fid;
  LOGSVR_FILE_LOCK;
  if (!Log_SrvGetFileType(type, &fid)) {
    LOGSVR_FILE_UNLOCK;
    return false;
  }
  g_LogServer->config.print_end[fid] = print_end;
  LOGSVR_FILE_UNLOCK;
  return true;
}

bool Log_SetRemoteServer(uint32 ip, uint16 port) {
  if (!g_LogServer) {
    printf("error:logSrv uninit, Log_SetRemoteServer failed\n");
    return false;
  }
  LOGSVR_FILE_LOCK;
  if (ip != g_LogServer->config.ip || port != g_LogServer->config.port) {
    // todo: Remote sender
    g_LogServer->config.ip = ip;
    g_LogServer->config.port = port;

#if defined(POSIX)
    bzero(&g_logsvr_status.udp_target_sockaddr, sizeof(struct sockaddr_in));
#endif
    g_logsvr_status.udp_target_sockaddr.sin_family = AF_INET;
    g_logsvr_status.udp_target_sockaddr.sin_port = htons(port);
    g_logsvr_status.udp_target_sockaddr.sin_addr.s_addr = htonl(ip);
  }
  LOGSVR_FILE_UNLOCK;
  return true;
}

bool Log_RelCacheCfg(int type, uint16 cache_size, uint16 time_out) {
  if (!g_LogServer) {
    printf("error:logSrv uninit, Log_RelCacheCfg failed\n");
    return false;
  }
  LOG_SRV_FILE_TYPE_E fid;
  uint32 timeu32 = time_out;
  if (!Log_SrvGetFileType(type, &fid)) {
    return false;
  }

  LOGSVR_FILE_LOCK;
  if (g_LogServer->config.cache_maxsize[fid] != cache_size) {
    free(g_logsvr_status.cache[fid]);
    g_logsvr_status.cache[fid] =
        (uint8 *)malloc(g_LogServer->config.cache_maxsize[fid]);
    if (g_logsvr_status.cache[fid]) {
      g_LogServer->config.cache_maxsize[fid] = cache_size;
    } else {
      printf("[log]log server remallocc size[%d] error!!!", cache_size);
      g_LogServer->config.cache_maxsize[fid] = 0;
    }
  }
  timeu32 *= 1000;
  timeu32 /= g_LogServer->config.pull_wait_ms;
  if (timeu32 > 0xffff) {
    timeu32 = 0xffff;
  }
  g_LogServer->config.write_overtime_cnt[fid] = (uint16)timeu32;
  g_logsvr_status.write_overtime_cnt[fid] = (uint16)timeu32;
  LOGSVR_FILE_UNLOCK;
  return true;
}

bool Log_SrvConfigSet(LOG_SRV_CONFIG_S *config) {
  if (!g_LogServer) {
    printf("error:logSrv uninit, Log_SrvConfigSet failed\n");
    return false;
  }

  Log_SetRemoteServer(config->ip, config->port);
  LOGSVR_FILE_LOCK;
  for (int i = 0; i < LOG_SRV_FILE_TYPE_MAX; i++) {
    if (g_logsvr_status.cache[i] != NULL &&
        g_LogServer->config.cache_maxsize[i] != config->cache_maxsize[i]) {
      free(g_logsvr_status.cache[i]);
      g_logsvr_status.cache[i] = NULL;
      if (config->cache_maxsize <= 0) {
        continue;
      }
    }
    g_logsvr_status.cache[i] = (uint8 *)malloc(config->cache_maxsize[i]);
    if (g_logsvr_status.cache[i] == NULL) {
      printf("[log]log server cache malloc error!!!\n");
      while (i--) {
        if (g_logsvr_status.cache[i] != NULL) {
          free(g_logsvr_status.cache[i]);
          g_logsvr_status.cache[i] = NULL;
        }
      }
      LOGSVR_FILE_UNLOCK;
      return false;
    }
  }
  memcpy(&g_LogServer->config, config, sizeof(LOG_SRV_CONFIG_S));
  LogPrintEnd = g_LogServer->config.print_end;
  if (g_LogServer->config.pull_wait_ms < 10) {
    g_LogServer->config.pull_wait_ms = 10;  // thread sleep minval ms
  }
  for (int i = 0; i < LOG_SRV_FILE_TYPE_MAX; i++) {
    if (g_LogServer->config.write_overtime_cnt[i] == 0) {
      g_LogServer->config.write_overtime_cnt[i] = 1;
    }
    g_logsvr_status.write_overtime_cnt[i] =
        g_LogServer->config.write_overtime_cnt[i];
  }
  if (g_LogServer->config.foldpath[0] == '\0') {
    snprintf(g_LogServer->config.foldpath, LOG_FOlDPATH_MAXLEN,
             kLogFoldPathDefault);
  }
  // 创建文件夹
  Log_SrvFoldCreate(g_LogServer->config.foldpath);

  Log_SrvCheckFileIndex();
  LOGSVR_FILE_UNLOCK;
  return true;
}

int Log_SvrWrite(LOG_SRV_FILE_TYPE_E fid) {
  char data_name[LOG_FILFPATH_MAXLEN];
  char index_name[LOG_FILFPATH_MAXLEN];
  char f_buf[4] = "rb+";
  uint8 *p;
  LOG_NODE_FILE_S *item;
  uint32 node_len;

  LOGSVR_FILE_LOCK;
  if (g_logsvr_status.cache_len[fid] == 0) {
    LOGSVR_FILE_UNLOCK;
    return 0;
  }
  // check file max size
  if (g_logsvr_status.next_need_write[fid]) {
    g_logsvr_status.next_need_write[fid] = false;
    g_logsvr_status.cur_file_len[fid] = 0;
    f_buf[0] = 'w';
    f_buf[1] = 'b';
    f_buf[2] = '+';
    f_buf[3] = '\0';
  } else {
    f_buf[0] = 'r';
    f_buf[1] = 'b';
    f_buf[2] = '+';
    f_buf[3] = '\0';
  }
  // get file name
  Log_SrvGetFileNameCurrent(fid, data_name, index_name, LOG_FILFPATH_MAXLEN);
  // open file
  FILE *fp_data = fopen(data_name, f_buf);
  if (fp_data == NULL) {
    LOGSVR_FILE_UNLOCK;
    printf("[log]log server open file(%s) error!!!\n", data_name);
    return -2;
  }
  fseek(fp_data, 0, SEEK_END);
  int data_offset = ftell(fp_data);
  if (data_offset < 0) {
    fclose(fp_data);
    FILE *fp_index = fopen(index_name, "wb");
    if (fp_index == NULL) {
      LOGSVR_FILE_UNLOCK;
      printf("[log]log server open file(%s) error!!!\n", index_name);
      return -3;
    }
    fclose(fp_index);
    fp_data = fopen(data_name, "wb");
    if (fp_data == NULL) {
      LOGSVR_FILE_UNLOCK;
      printf("[log]log server open file(%s) error!!!\n", data_name);
      return -4;
    }
    g_logsvr_status.cur_file_len[fid] = 0;
  } else {
    g_logsvr_status.cur_file_len[fid] = data_offset;
  }
  FILE *fp_index = fopen(index_name, f_buf);
  if (fp_index == NULL) {
    LOGSVR_FILE_UNLOCK;
    printf("[log]log server open file(%s) error!!!\n", index_name);
    fclose(fp_data);
    return -5;
  }
  // write file
  p = g_logsvr_status.cache[fid];
  fwrite(p, 1, g_logsvr_status.cache_len[fid], fp_data);
  fflush(fp_data);
  node_len = 0;
  fseek(fp_index, 0, SEEK_END);
  int index_offset = ftell(fp_index);
  if (index_offset > 0 && index_offset & 0x3) {
    index_offset &= 0xfffffffc;
    fseek(fp_index, index_offset, SEEK_SET);
  }
  for (uint32 i = 0; i < g_logsvr_status.cache_len[fid];) {
    item = (LOG_NODE_FILE_S *)p;
    if (item->head != LOG_FILE_NODE_HEAD) {
      break;
    }
    //  if (i == 0 && g_logsvr_status.cur_file_len[fid] == 0) {
    //    printf("[log]log server file(%d-%d) length==0.\n"
    //           , fid, g_logsvr_status.cur_file_id[fid]);
    //  }
    uint32 offset = g_logsvr_status.cur_file_len[fid] + i;
    fwrite(&offset, 1, 4, fp_index);
    node_len = 1 + 4 + item->node.length + 2;
    p += node_len;
    i += node_len;
  }
  fflush(fp_index);
  g_logsvr_status.cur_file_len[fid] += g_logsvr_status.cache_len[fid];
  g_logsvr_status.cache_len[fid] = 0;
  // close file
  fclose(fp_data);
  fclose(fp_index);
  g_logsvr_status.write_overtime_cnt[fid] =
      g_LogServer->config.write_overtime_cnt[fid];
  LOGSVR_FILE_UNLOCK;
  return 0;
}
#if defined(LOG_PAUSE) 
int Log_SendCmd(const char *str, uint16_t port) {
  ssize_t count;
  int client_fd;
  struct sockaddr_in ser_addr;

  client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(client_fd < 0)
  {
    printf("create socket fail %s!\n", strerror(errno));
    return -1;
  }

  memset(&ser_addr, 0, sizeof(ser_addr));
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  ser_addr.sin_port = htons(port);

  count = sendto(client_fd, (const void *)str, strlen(str), MSG_DONTWAIT,
                      (const struct sockaddr *)&ser_addr, sizeof(ser_addr));
  if(count == -1) {
    printf("sendto error %s\n", strerror(errno));
  }
  printf("send cmd %s failed\n", str);
  close(client_fd);

  return 0;
}



void Log_Pause(void) {
  if (g_LogServer) {
    Log_FlushCache();
    g_logsvr_status.paused = true;
    printf("Log_Pause\n");
    return;
  } else {
    Log_SendCmd("pause", LOG_SRV_CMD_PORT);
  }
}


void Log_Resume(void) {
  if (g_LogServer) {
    g_logsvr_status.paused = false;
    printf("Log_Resume in server\n");
    return;
  } else {
    Log_SendCmd("resume", LOG_SRV_CMD_PORT);
  }
}
#endif
void Log_FlushCache(void) {
  if (!g_LogServer) {
    printf("error:logSrv uninit, Log_FlushCache failed\n");
    return;
  }
  g_logsvr_status.flush_cache = true;
  for (int i = 0; i < 400; i++) {
    chml::Thread::SleepMs(10);
    if (!g_logsvr_status.flush_cache) break;
  }
}

void Log_SvrReadyOutputString(LOG_NODE_S *node) {
  chml::TimeS tm;
  chml::XTimeUtil::TimeLocalMake(&tm, node->sec);
  uint8 *p = ((uint8 *)node) + sizeof(LOG_NODE_S);
  const char *kLogLevelString[] = {"DEBUG", "INFO", "WARNING",
                                   "ERROR", "KEY",  "-"};

  switch (node->level) {
    case LL_DEBUG:
    case LL_INFO:
    case LL_ERROR:
    case LL_WARNING:
    case LL_KEY:
      snprintf(g_logsvr_status.output_buf, 1020,
               "%04d-%02d-%02d %02d:%02d:%02d,%-6d,[%s]: %s", tm.year, tm.month,
               tm.day, tm.hour, tm.min, tm.sec, node->usec,
               kLogLevelString[node->level], p);
      break;
    case LL_NONE:
      snprintf(g_logsvr_status.output_buf, 1020,
               "%04d-%02d-%02d %02d:%02d:%02d,%-6d: %s", tm.year, tm.month,
               tm.day, tm.hour, tm.min, tm.sec, node->usec, p);
      break;
  }
}

void Log_SvrPrintNode(LOG_NODE_S *node) {
#if defined(WIN32)
#define LPN_COLOR_RED (FOREGROUND_RED)
#define LPN_COLOR_GREEN (FOREGROUND_GREEN)
#define LPN_COLOR_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define LPN_COLOR_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

  switch (node->level) {
    case LL_ERROR:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_RED);
      printf("%s\n", g_logsvr_status.output_buf);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
      break;
    case LL_WARNING:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_YELLOW);
      printf("%s\n", g_logsvr_status.output_buf);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
      break;
    case LL_KEY:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_GREEN);
      printf("%s\n", g_logsvr_status.output_buf);
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                              FOREGROUND_INTENSITY | LPN_COLOR_WHITE);
      break;
    case LL_NONE:
    default:
      printf("%s\n", g_logsvr_status.output_buf);
      break;
  }
#else
  switch (node->level) {
    case LL_ERROR:
      printf("\033[1;31;40m%s\033[0m\n", g_logsvr_status.output_buf);
      break;
    case LL_WARNING:
      printf("\033[1;33;40m%s\033[0m\n", g_logsvr_status.output_buf);
      break;
    case LL_KEY:
      printf("\033[1;32;40m%s\033[0m\n", g_logsvr_status.output_buf);
      break;
    case LL_NONE:
    default:
      printf("%s\n", g_logsvr_status.output_buf);
      break;
  }
#endif
}

void Log_SvrUdpSendNode() {
  static int id = 0;
  char send_buff[1024];
  if (g_LogServer->config.ip != 0) {
    ++id;
    strcpy(send_buff, g_logsvr_status.output_buf);
    snprintf(g_logsvr_status.output_buf, 1024, "%d:%s", id, send_buff);
    int slen = strlen(g_logsvr_status.output_buf);
    // g_logsvr_status.output_buf[slen++] = '\r';
    g_logsvr_status.output_buf[slen++] = '\n';
    g_logsvr_status.output_buf[slen++] = '\0';

    sendto(g_logsvr_status.udp_sockfd, g_logsvr_status.output_buf, slen, 0,
           (struct sockaddr *)&g_logsvr_status.udp_target_sockaddr,
           sizeof(g_logsvr_status.udp_target_sockaddr));
  }
}

void Log_SvrClearSingleFile(LOG_SRV_FILE_TYPE_E file_type, int subfile_index) {
  char data_name[LOG_FILFPATH_MAXLEN];
  char index_name[LOG_FILFPATH_MAXLEN];
  FILE *data_file;
  FILE *index_file;

  Log_SrvGetFileName(file_type, subfile_index, data_name, index_name,
                     LOG_FILFPATH_MAXLEN);

  do {
    data_file = fopen(data_name, "wb");
    index_file = fopen(index_name, "wb");

    if (data_file == NULL) {
      printf("[log]:%s file clear false!!!", data_name);
    }
    if (index_file == NULL) {
      printf("[log]:%s file clear false!!!", index_name);
    }

    if (data_file) {
      fclose(data_file);
      data_file = NULL;
    }
    if (index_file) {
      fclose(index_file);
      index_file = NULL;
    }
  } while (0);
}

void Log_SvrClear(void) {
  LOGSVR_FILE_LOCK;

  g_logsvr_status.cur_log_id = 1;

  for (int itype = 0; itype < LOG_SRV_FILE_TYPE_MAX; itype++) {
    // 清空文件
    for (int i = 0; i < LOG_FILF_GROUP_MAXCNT; i++) {
      Log_SvrClearSingleFile((LOG_SRV_FILE_TYPE_E)itype, i);
    }
    g_logsvr_status.cur_file_len[itype] = 0;
    // 清空二级缓存
    g_logsvr_status.cache_len[itype] = 0;
  }
  LOGSVR_FILE_UNLOCK;
}

void Log_SvrPullFromClient(LOG_NODE_S *node) {
  LOG_SRV_FILE_TYPE_E fid;
  uint8 *p = NULL;
  if (node != NULL) {
    // printf("*** NODE[T=%d, L=%d] ***\n", node->type, node->length);
    int file_node_size;
    // parse node
    file_node_size = 1 + 4 + node->length + 2;
    p = (uint8 *)node;
    if (node->length > LOG_RELEASE_MAX_LEN) {
      node->length = LOG_RELEASE_MAX_LEN;
      p[node->length - 1] = '\0';
    }
    if (!Log_SrvGetFileType((LOG_TYPE_E)node->type, &fid)) {
      return;
    }
    if (g_LogServer->config.print_end[fid] & (LE_LOCAL | LE_REMOTE)) {
      Log_SvrReadyOutputString(node);
      if (g_LogServer->config.print_end[fid] & LE_LOCAL) {
        Log_SvrPrintNode(node);
      }
      if (g_LogServer->config.print_end[fid] & LE_REMOTE) {
        Log_SvrUdpSendNode();
      }
    }
    if (g_LogServer->config.print_end[fid] & LE_FILE) {
      // file overflow
      if (g_logsvr_status.cur_file_len[fid] + g_logsvr_status.cache_len[fid] +
              file_node_size >
          g_LogServer->config.file_maxlen[fid]) {
        g_logsvr_status.cur_file_id[fid] =
            (g_logsvr_status.cur_file_id[fid] + 1) % LOG_FILF_GROUP_MAXCNT;
        g_logsvr_status.next_need_write[fid] = true;
        g_logsvr_status.cur_file_len[fid] = 0;
      }
      // Lack of free cache, write first
      if (g_logsvr_status.cache_len[fid] + file_node_size >
          g_LogServer->config.cache_maxsize[fid]) {
        if (g_logsvr_status.cache_len[fid] > 0) {
          Log_SvrWrite(fid);
          g_logsvr_status.cache_len[fid] = 0;
        } else {
          printf("[log]log server node too long = %d!!!\n", node->length);
          return;
        }
      }
      if (g_logsvr_status.cur_log_id > 0xfffffff0) {
        Log_SvrClear();
      }
      // Record cache
      p = g_logsvr_status.cache[fid] + g_logsvr_status.cache_len[fid];
      *p++ = LOG_FILE_NODE_HEAD;
      g_logsvr_status.cur_log_id++;
      memcpy(p, (uint8 *)&(g_logsvr_status.cur_log_id), 4);
      p += 4;
      memcpy(p, (uint8 *)node, node->length);
      p += node->length;
      *p++ = (uint8)(node->length);
      *p++ = (uint8)(node->length >> 8);
      p += 2;
      g_logsvr_status.cache_len[fid] += file_node_size;
      g_logsvr_status.last_time.sec = node->sec;
      g_logsvr_status.last_time.usec = node->usec;
    }
  }
}

bool ThreadConfig(void) {
#if defined(WIN32)
  // Config thread name
  // As seen on MSDN.
  // http://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.71).aspx
#define MSDEV_SET_THREAD_NAME 0x406D1388
  typedef struct tagTHREADNAME_INFO {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  } THREADNAME_INFO;

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = "ml_logSrv";
  info.dwThreadID = g_LogServer->thread_id_;
  info.dwFlags = 0;

  __try {
    RaiseException(MSDEV_SET_THREAD_NAME, 0, sizeof(info) / sizeof(DWORD),
                   reinterpret_cast<ULONG_PTR *>(&info));
  }
#ifndef __MINGW32__
  __except(EXCEPTION_CONTINUE_EXECUTION)
#else
  __catch(...)
#endif
  {
  }

  // Config thread periority
  ::SetThreadPriority(g_LogServer->thread_, LOG_SERVER_PRIORITY);
  return true;
#elif defined(POSIX)
  // Config thread name
  prctl(PR_SET_NAME, "ml_logSrv");
  //绑定到CPU0
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  if (sched_setaffinity(0, sizeof(mask), &mask) == -1) { //设置线程CPU亲和力
    printf("[log]error:could not set CPU affinity...\n");
  }

  // Config thread periority
#ifdef LITEOS
  struct sched_param param;
  param.sched_priority = LOG_SERVER_PRIORITY;
  if (pthread_setschedparam(pthread_self(), SCHED_RR, &param)) {
    printf("[log]error:set thread priority failed\n");
    return false;
  }
#endif
  return true;
#endif
  return false;
}

void *ThreadProc(void *usr_data) {
  (void)ThreadConfig();

  while (!(g_LogServer->stop_flag)) {

#ifdef LOG_PAUSE

    if(g_logsvr_status.cmd_sockfd >= 0) {
      Log_SrvRecvPack(g_LogServer->config.pull_wait_ms);
    }
    else {
      chml::Thread::SleepMs(g_LogServer->config.pull_wait_ms);
    }
#else
    chml::Thread::SleepMs(g_LogServer->config.pull_wait_ms);
#endif

    // Processing log
    LOG_NODE_S *cache = (LOG_NODE_S *)Log_CacheGet();
    while (cache) {
      //(void)printf("%d,%d,%s\n", cache->sec, cache->usec, cache->body);
      Log_SvrPullFromClient(cache);
      (void)Log_CacheFree((void *)cache);
      cache = (LOG_NODE_S *)Log_CacheGet();
    }
#ifdef LOG_PAUSE
    if (g_logsvr_status.paused == true) {
      continue;
    }
#endif
    // Timer arrives
    if (g_logsvr_status.flush_cache) {
      g_logsvr_status.flush_cache = false;
      for (int i = 0; i < LOG_SRV_FILE_TYPE_MAX; i++) {
        g_logsvr_status.write_overtime_cnt[i] = 0;
      }
    }
    for (int i = 0; i < LOG_SRV_FILE_TYPE_MAX; i++) {
      if (g_logsvr_status.write_overtime_cnt[i] == 0) {
        if (g_logsvr_status.cache_len[i] > 0) {
          Log_SvrWrite((LOG_SRV_FILE_TYPE_E)i);
          g_logsvr_status.cache_len[i] = 0;

          g_logsvr_status.write_overtime_cnt[i] =
              g_LogServer->config.write_overtime_cnt[i];
        }
      } else {
        g_logsvr_status.write_overtime_cnt[i]--;
      }
    }
  }
  printf("[log]log server thread exit!!!\n");
  free(g_LogServer);
  g_LogServer = NULL;
  return NULL;
}

bool ThreadCreate(void) {
#if defined(WIN32)
  g_LogServer->thread_ =
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, 0,
                   &(g_LogServer->thread_id_));
  if (!g_LogServer->thread_) {
    printf("[log]error:create thread failed\n");
    return false;
  }
#elif defined(POSIX)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  int ret = pthread_create(&(g_LogServer->thread_), &attr, ThreadProc, NULL);
  if (0 != ret) {
    printf("[log]error:create thread failed, err:%d\n", ret);
    return false;
  }
#endif
  return true;
}

void Log_SrvDeinit(void) {
  if (g_LogServer) {
    g_LogServer->stop_flag = true;
    printf("[log]deinit log server\n");
  }
}

#ifdef LOG_PAUSE
int Log_SrvRecvPack(uint32_t timeout) {
  int ret = 0;
  socklen_t len;
  char buf[128];
  ssize_t count = 0;
  struct pollfd pds[1];
  struct sockaddr_in clent_addr;

  pds[0].fd = g_logsvr_status.cmd_sockfd;
  pds[0].events = POLLIN;
  pds[0].revents = 0;

  ret = poll(pds, 1, timeout);
  if(ret == 0) {
    return 0;
  }

  if(ret < 0) {
     printf("poll error:%s!\n", strerror(errno));
     g_logsvr_status.cmd_sockfd = -1;
     return -1;
  }
  len = sizeof(clent_addr);
  memset(buf, 0, sizeof(buf));
  count = recvfrom(g_logsvr_status.cmd_sockfd, buf, sizeof(buf) - 1, MSG_DONTWAIT, (struct sockaddr*)&clent_addr, &len);
  if(count == -1)
  {
    if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
      return 0;
    }
    printf("recvfrom error:%s!\n", strerror(errno));
    g_logsvr_status.cmd_sockfd = -1;
    return -1;
  }
  printf("LOG_SER recv cmd %s\n", buf);
  if (strncmp(buf, "pause", sizeof(buf)) == 0) {
    g_logsvr_status.paused = true;
  } else if (strncmp(buf, "resume", sizeof(buf)) == 0) {
    g_logsvr_status.paused = false;
  } 

  return 0;
}

int Log_SrvInitCmdSocket(uint16_t port) {
  struct sockaddr_in server;

  if ((g_logsvr_status.cmd_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("[log]error:log service cmd_sockfd failed\n");
    return -1;
  }

  memset(&server,0,sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(port);
	
  if (-1==bind(g_logsvr_status.cmd_sockfd,(struct sockaddr *)&server,sizeof(struct sockaddr)))
	{
		printf("bind error %s\n", strerror(errno));
    close(g_logsvr_status.cmd_sockfd);
		return -1;
	}
/*
  int flag = fcntl(sockfd, F_GETFL, 0);
  if (flag < 0) {
    printf("fcntl F_GETFL fail %s\n", strerror(errno));
    return;
  }
  if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) { 
    printf("fcntl F_SETFL fail %s\n", strerror(errno));
  }
*/
  return 0;
}
#endif
bool Log_SrvInit(LOG_SRV_CONFIG_S *config) {
  if (g_LogServer) {
    printf("[log]log server already inited\n");
    return true;
  }
  memset(&g_logsvr_status, 0, sizeof(LOG_SRV_STATUS_S));
  LOGSVR_MUTEX_INIT(g_log_srv_mutex);

  do {
    int size = sizeof(LOG_SRV_ENTITY_S);
    g_LogServer = (LOG_SRV_ENTITY_S *)malloc(size);
    if (NULL == g_LogServer) {
      printf("[log]error:malloc log entity failed, size:%d\n", size);
      break;
    }
    memset(g_LogServer, 0x00, size);
    if (!Log_SrvConfigSet(config)) {
      printf("[log]error:log service config set failed\n");
      break;
    }
    g_logsvr_status.output_buf = (char *)malloc(1024);
    if (NULL == g_logsvr_status.output_buf) {
      printf("[log]error:malloc log print buffer failed, size:1024\n");
      free(g_LogServer);
      g_LogServer = NULL;
      break;
    }
#if defined(WIN32)
    WORD socketVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(socketVersion, &wsaData) != 0) {
      return 0;
    }
    g_logsvr_status.udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    if ((g_logsvr_status.udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      printf("[log]error:log service socket failed\n");
    }
#ifdef LOG_PAUSE
    if (Log_SrvInitCmdSocket(LOG_SRV_CMD_PORT) < 0) {
      printf("[log]error:log service cmd_sockfd failed\n");
      g_logsvr_status.cmd_sockfd = -1;
    }
#endif
#endif

    bool ret = ThreadCreate();
    if (!ret) {
      printf("[log]error:log service init failed\n");
      break;
    }
    return true;
  } while (0);

  Log_SrvDeinit();
  return false;
}
