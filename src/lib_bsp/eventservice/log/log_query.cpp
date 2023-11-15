
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "boost/boost_settings.hpp"
#include "eventservice/base/basictypes.h"

#include "eventservice/base/criticalsection.h"
#include "eventservice/base/sigslot.h"
#include "eventservice/util/time_util.h"
#include "eventservice/log/log_client.h"
#include "eventservice/log/log_server.h"

typedef struct {
  int file_idx;  //打开的文件(0代表A, 1代表B)
  bool is_first_file;
  FILE *fpidx[2], *fpfile[2];

  int cur_index;    //索引的下标
  int max_index;    //当前文件索引下标最大值
  int file_offset;  //索引指向文件offset

  bool hasres;    //此类文件是否还有结果
  bool isopened;  //此类文件是否打开了文件
  LOG_NODE_FILE_S data;
} LOG_FILE_QUERY_S;

typedef uint32 LOG_INDEX_VALUE;
#define LOG_TYPE_E_MAX_SIZE (8)
static bool useCustomFolder = false;
static char customFolder[LOG_FILFPATH_MAXLEN];

// a < b return -1
// a = b return 0
// a > b return 1
static int log_CompareTimeS(const LOG_TIME_S &a, const LOG_TIME_S &b) {
  if (a.sec < b.sec) {
    return -1;
  } else if (a.sec > b.sec) {
    return 1;
  } else if (a.usec < b.usec) {
    return -1;
  } else if (a.usec > b.usec) {
    return 1;
  }
  return 0;
}

static bool log_LoadFileFirstRecordDesc(LOG_FILE_QUERY_S &cur,
                                        const uint32 st_id,
                                        const uint32 ed_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false;  //无数据
  }

  int len = offset / sizeof(LOG_INDEX_VALUE);
  int tmp_len = len;

  int fi = len - 1, half = 0;
  LOG_INDEX_VALUE tmp_idx;
  LOG_NODE_FILE_S tmp_lnfs;
  while (len) {
    half = len >> 1;

    fseek(fpidx, (fi - half) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
      DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
      return false;
    }
    fseek(fpfile, tmp_idx, SEEK_SET);
    if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp_lnfs.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    if (tmp_lnfs.id > ed_id) {
      len = len - half - 1;
      fi = fi - half - 1;
    } else {
      len = half;
    }
  }
  if (fi == -1) {
    return false;  //无结果
  }
  //有结果
  fseek(fpidx, (fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
  if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
    DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
    return false;
  }
  fseek(fpfile, tmp_idx, SEEK_SET);
  if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
    DLOG_WARNING(MOD_EB, "fread error: read data error\n");
    return false;
  } else if (tmp_lnfs.head != 0x47) {
    DLOG_WARNING(MOD_EB, "read data head error\n");
    return false;
  }
  if (tmp_lnfs.id < st_id) {
    //第一条记录不在范围内
    return false;
  }

  cur.cur_index = fi;
  cur.data = tmp_lnfs;
  cur.file_offset = tmp_idx;
  cur.max_index = tmp_len;
  return true;
}

static bool log_LoadFileFirstRecordAsc(LOG_FILE_QUERY_S &cur,
                                       const uint32 st_id, const uint32 ed_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false;  //无数据
  }

  int len = offset / sizeof(LOG_INDEX_VALUE);
  int tmp_len = len;

  int fi = 0, half = 0;
  LOG_INDEX_VALUE tmp_idx;
  LOG_NODE_FILE_S tmp_lnfs;
  while (len) {
    half = len >> 1;

    fseek(fpidx, (half + fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
      DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
      return false;
    }
    fseek(fpfile, tmp_idx, SEEK_SET);
    if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp_lnfs.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    if (tmp_lnfs.id < st_id) {
      len = len - half - 1;
      fi = fi + half + 1;
    } else {
      len = half;
    }
  }
  if (fi == tmp_len) {
    return false;  //无结果
  }
  //有结果
  fseek(fpidx, (fi) * sizeof(LOG_INDEX_VALUE), SEEK_SET);
  if (!fread(&tmp_idx, sizeof(LOG_INDEX_VALUE), 1, fpidx)) {
    DLOG_WARNING(MOD_EB, "fread_error: read index error\n");
    return false;
  }
  fseek(fpfile, tmp_idx, SEEK_SET);
  if (fread(&tmp_lnfs, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
    DLOG_WARNING(MOD_EB, "fread error: read data error\n");
    return false;
  } else if (tmp_lnfs.head != 0x47) {
    DLOG_WARNING(MOD_EB, "read data head error\n");
    return false;
  }
  if (tmp_lnfs.id > ed_id) {
    //第一条记录不在范围内
    return false;
  }

  cur.cur_index = fi;
  cur.data = tmp_lnfs;
  cur.file_offset = tmp_idx;
  cur.max_index = tmp_len;
  return true;
}

static bool log_FindCategoryFirstRecord(LOG_FILE_QUERY_S &cur,
                                        const uint32 min_id,
                                        const uint32 max_id,
                                        LOG_QUERY_TYPE qtype) {
  cur.is_first_file = true;
  LOG_FILE_QUERY_S fa, fb;
  fa = cur;
  fa.file_idx = 0;
  fb = cur;
  fb.file_idx = 1;
  int resa, resb;
  if (qtype == LOG_QUERY_ASC_TYPE) {
    resa = log_LoadFileFirstRecordAsc(fa, min_id, max_id);
    resb = log_LoadFileFirstRecordAsc(fb, min_id, max_id);
  } else {
    resa = log_LoadFileFirstRecordDesc(fa, min_id, max_id);
    resb = log_LoadFileFirstRecordDesc(fb, min_id, max_id);
  }
  if (resa && resb) {
    if (qtype == LOG_QUERY_ASC_TYPE) {
      if (fa.data.id < fb.data.id) {
        cur = fa;
      } else {
        cur = fb;
      }
    } else {
      if (fa.data.id > fb.data.id) {
        cur = fa;
      } else {
        cur = fb;
      }
    }
    return true;
  } else if (resa) {
    cur = fa;
    return true;
  } else if (resb) {
    cur = fb;
    return true;
  }
  return false;
}

static bool log_FindFileIdRange(LOG_FILE_QUERY_S &cur, const LOG_TIME_S &st,
                                const LOG_TIME_S &ed, uint32 &min_id,
                                uint32 &max_id) {
  FILE *fpfile = cur.fpfile[cur.file_idx];
  FILE *fpidx = cur.fpidx[cur.file_idx];

  fseek(fpidx, 0, SEEK_END);
  int offset = ftell(fpidx);
  if (offset == 0) {
    return false;  //无数据
  }
  min_id = ~0;
  max_id = 0;

  int len = offset / sizeof(LOG_INDEX_VALUE);
  LOG_NODE_FILE_S tmp;
  bool find_record = false;
  for (int i = 0; i < len; i++) {
    fseek(fpidx, i * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    int offset;
    if (fread(&offset, sizeof(LOG_INDEX_VALUE), 1, fpidx) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read index error\n");
      return false;
    }
    fseek(fpfile, offset, SEEK_SET);
    if (fread(&tmp, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return false;
    } else if (tmp.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head error\n");
      return false;
    }
    LOG_TIME_S t;
    t.sec = tmp.node.sec;
    t.usec = tmp.node.usec;
    if (log_CompareTimeS(st, t) <= 0 && log_CompareTimeS(t, ed) <= 0) {
      min_id = ML_MIN(min_id, tmp.id);
      max_id = ML_MAX(max_id, tmp.id);
      find_record = true;
    }
  }
  if (find_record) {
    return true;
  } else {
    return false;
  }
}

static bool log_FindCategoryIdRange(LOG_FILE_QUERY_S &cur, const LOG_TIME_S st,
                                    const LOG_TIME_S ed, uint32 &min_id,
                                    uint32 &max_id) {
  LOG_FILE_QUERY_S fa, fb;
  fa = cur;
  fa.file_idx = 0;
  fb = cur;
  fb.file_idx = 1;
  uint32 max_id_a, max_id_b;
  uint32 min_id_a, min_id_b;
  bool resa = log_FindFileIdRange(fa, st, ed, min_id_a, max_id_a);
  bool resb = log_FindFileIdRange(fb, st, ed, min_id_b, max_id_b);
  if (resa && resb) {
    min_id = ML_MIN(min_id_a, min_id_b);
    max_id = ML_MAX(max_id_a, max_id_b);
    return true;
  } else if (resa) {
    min_id = min_id_a;
    max_id = max_id_a;
    return true;
  } else if (resb) {
    min_id = min_id_b;
    max_id = max_id_b;
    return true;
  }
  return false;
}

static int log_LoadNextRecord(LOG_FILE_QUERY_S &cur, const uint32 min_id,
                              const uint32 max_id, LOG_QUERY_TYPE qtype) {
  if (qtype == LOG_QUERY_ASC_TYPE) {
    cur.cur_index++;
  } else {
    cur.cur_index--;
  }
  if (cur.cur_index >= 0 && cur.cur_index < cur.max_index) {
    //当前文件未读到末尾
    FILE *fpfile = cur.fpfile[cur.file_idx];
    FILE *fpidx = cur.fpidx[cur.file_idx];

    fseek(fpidx, cur.cur_index * sizeof(LOG_INDEX_VALUE), SEEK_SET);
    if (fread(&cur.file_offset, sizeof(LOG_INDEX_VALUE), 1, fpidx) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read index error\n");
      return 0;
    }
    fseek(fpfile, cur.file_offset, SEEK_SET);
    if (fread(&cur.data, sizeof(LOG_NODE_FILE_S), 1, fpfile) != 1) {
      DLOG_WARNING(MOD_EB, "fread error: read data error\n");
      return 0;
    } else if (cur.data.head != 0x47) {
      DLOG_WARNING(MOD_EB, "read data head is error\n");
      return 0;
    }
    if (cur.data.id >= min_id && cur.data.id <= max_id) {
      return 1;
    } else {
      return 0;
    }
  } else {
    //当前文件已读到末尾
    if (!cur.is_first_file) {
      return 0;
    }
    cur.file_idx = 1 - cur.file_idx;
    cur.is_first_file = false;

    FILE *fpfile = cur.fpfile[cur.file_idx];
    FILE *fpidx = cur.fpidx[cur.file_idx];
    fseek(fpidx, 0, SEEK_END);
    cur.max_index = ftell(fpidx) / sizeof(LOG_INDEX_VALUE);
    if (cur.max_index == 0) {
      //下一个文件无信息
      return 0;
    } else {
      //下一个文件有信息
      if (qtype == LOG_QUERY_ASC_TYPE) {
        cur.cur_index = -1;
      } else {
        cur.cur_index = cur.max_index;
      }
      return log_LoadNextRecord(cur, min_id, max_id, qtype);
    }
  }
  return 0;
}

static bool log_openCategoryFiles(LOG_FILE_QUERY_S &cur, char *filepath[]) {
  for (int i = 0; i < 4; i++) {
    FILE **fpp;
    if (i == 0) {
      fpp = &cur.fpfile[0];
    } else if (i == 1) {
      fpp = &cur.fpidx[0];
    } else if (i == 2) {
      fpp = &cur.fpfile[1];
    } else {
      fpp = &cur.fpidx[1];
    }
    *fpp = fopen(filepath[i], "rb");
    if (*fpp == NULL) {
      DLOG_WARNING(MOD_EB, "open file %s error\n", filepath[i]);
      *fpp = fopen(filepath[i], "wb+");
      if (*fpp == NULL) {
        DLOG_WARNING(MOD_EB, "create file %s error\n", filepath[i]);
        return false;
      }
    }
  }
  return true;
}

bool Log_SearchUseCustomFolder(const char folder[128], int len) {
  useCustomFolder = true;
  if (len >= LOG_FOlDPATH_MAXLEN) {
    len = LOG_FOlDPATH_MAXLEN - 1;
  }
  memcpy(customFolder, folder, len);
  customFolder[len] = '\0';
  return true;
}

static void log_GetCustomFileName(LOG_SRV_FILE_TYPE_E fid, int index,
                                  char *out_data_name, char *out_index_name,
                                  int maxlen) {
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

  foldpath = customFolder;

  snprintf(out_data_name, maxlen, sformat_data[fid], foldpath, index);
  snprintf(out_index_name, maxlen, sformat_index[fid], foldpath, index);
}

static bool log_CloseAllFiles(LOG_FILE_QUERY_S arr[], int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i].fpfile[0] != NULL) {
      fclose(arr[i].fpfile[0]);
    }
    if (arr[i].fpfile[1] != NULL) {
      fclose(arr[i].fpfile[1]);
    }
    if (arr[i].fpidx[0] != NULL) {
      fclose(arr[i].fpidx[0]);
    }
    if (arr[i].fpidx[1] != NULL) {
      fclose(arr[i].fpidx[1]);
    }
  }
  return true;
}

static int log_OpenTypeFiles(LOG_FILE_QUERY_S arr[], int size,
                             const uint8 type_mask) {
  char file_name[4][LOG_FILFPATH_MAXLEN];
  char *file_name_array[4] = {file_name[0], file_name[1], file_name[2],
                              file_name[3]};
  for (int i = 0; i < size; i++) {
    arr[i].fpfile[0] = arr[i].fpfile[1] = NULL;
    arr[i].fpidx[0] = arr[i].fpidx[1] = NULL;
  }
  for (int i = 0; i < size; i++) {
    uint8 cur_type;
    if (i == 0) {
      cur_type = LT_DEBUG;
    } else if (i == 1) {
      cur_type = LT_POINT;
    } else if (i == 2) {
      cur_type = LT_SYS;
    } else if (i == 3) {
      cur_type = LT_DEV;
    } else if (i == 4) {
      cur_type = LT_UI;
    } else if (i == 5) {
      cur_type = LT_USER1;
    } else if (i == 6) {
      cur_type = LT_USER2;
    } else if (i == 7) {
      cur_type = LT_USER3;
    } else {
      cur_type = 0;
    }
    arr[i].hasres = false;
    arr[i].isopened = false;
    if (!(type_mask & cur_type)) {
      //未查询当前type;
      continue;
    }
    LOG_SRV_FILE_TYPE_E ftype;
    if (!Log_SrvGetFileType((LOG_TYPE_E)cur_type, &ftype)) {
      continue;
    }
    if (useCustomFolder) {
      log_GetCustomFileName(ftype, 0, file_name[0], file_name[1],
                            LOG_FILFPATH_MAXLEN);
      log_GetCustomFileName(ftype, 1, file_name[2], file_name[3],
                            LOG_FILFPATH_MAXLEN);
    } else {
      Log_SrvGetFileName(ftype, 0, file_name[0], file_name[1],
                         LOG_FILFPATH_MAXLEN);
      Log_SrvGetFileName(ftype, 1, file_name[2], file_name[3],
                         LOG_FILFPATH_MAXLEN);
    }
    arr[i].isopened = true;
    if (!log_openCategoryFiles(arr[i], file_name_array)) {
      //打开文件失败 异常，退出。
      log_CloseAllFiles(arr, size);
      return false;
    }
  }
  return true;
}

static bool log_GetIdRange(LOG_FILE_QUERY_S arr[], int size,
                           const LOG_TIME_S start, const LOG_TIME_S end,
                           LOG_QUERY_NODE &qnode) {
  if (!qnode.is_first) {
    //非第一次访问，使用用户传入id的范围缓存，不需要计算。
    return true;
  }
  qnode.min_id = ~0;
  qnode.max_id = 0;
  bool hasres = false;
  for (int i = 0; i < size; i++) {
    uint32 tmp_min_id, tmp_max_id;
    if (!arr[i].isopened) {
      continue;
    }
    if (log_FindCategoryIdRange(arr[i], start, end, tmp_min_id, tmp_max_id)) {
      qnode.max_id = ML_MAX(qnode.max_id, tmp_max_id);
      qnode.min_id = ML_MIN(qnode.min_id, tmp_min_id);
      hasres = true;
    }
  }
  return hasres;
}

int Log_Search(const uint8 type_mask, const LOG_TIME_S start,
               const LOG_TIME_S end, const uint32 size,
               char buffer[][LOG_RELEASE_MAX_LEN], LOG_QUERY_NODE &qnode) {
  LOG_FILE_QUERY_S arr[LOG_TYPE_E_MAX_SIZE];
  int res_type_count = 0;

  chml::TimeS start_time, end_time;
  chml::XTimeUtil::TimeLocalMake(&start_time, start.sec);
  chml::XTimeUtil::TimeLocalMake(&end_time, end.sec);

  DLOG_INFO(MOD_EB,
            "Log search req,start:%4d%02d%02d %02d:%02d:%02d,"
            "end:%4d%02d%02d %02d:%02d:%02d,count:%d",
            start_time.year, start_time.month, start_time.day, start_time.hour,
            start_time.min, start_time.sec, end_time.year, end_time.month,
            end_time.day, end_time.hour, end_time.min, end_time.sec, size);

  //加载文件
  if (!useCustomFolder) {
    LOGSVR_FILE_LOCK;
  }
  if (!log_OpenTypeFiles(arr, LOG_TYPE_E_MAX_SIZE, type_mask)) {
    if (!useCustomFolder) {
      LOGSVR_FILE_UNLOCK;
    }
    return 0;
  }
  //获取id范围
  if (!log_GetIdRange(arr, LOG_TYPE_E_MAX_SIZE, start, end, qnode)) {
    log_CloseAllFiles(arr, LOG_TYPE_E_MAX_SIZE);
    if (!useCustomFolder) {
      LOGSVR_FILE_UNLOCK;
    }
    return 0;
  }
  //加载文件对应数据
  uint32 cur_min, cur_max;
  if (qnode.is_first) {
    if (qnode.qtype == LOG_QUERY_ASC_TYPE) {
      qnode.start_id = 0;
    } else {
      qnode.start_id = ~(uint32)0;
    }
  }
  if (qnode.qtype == LOG_QUERY_ASC_TYPE) {
    cur_min = ML_MAX(qnode.min_id, qnode.start_id + 1);
    cur_max = qnode.max_id;
  } else {
    cur_min = qnode.min_id;
    cur_max = ML_MIN(qnode.max_id, qnode.start_id - 1);
  }
  for (int i = 0; i < LOG_TYPE_E_MAX_SIZE; i++) {
    if (arr[i].isopened) {
      if (log_FindCategoryFirstRecord(arr[i], cur_min, cur_max, qnode.qtype)) {
        arr[i].hasres = true;
        res_type_count++;
      }
    }
  }

  //多路归并并输出到Buff
  uint32 added = 0;
  while (added < size && res_type_count != 0) {
    //找最小的id
    int idx;
    uint32 id;
    bool update_id = false;
    for (int i = 0; i < LOG_TYPE_E_MAX_SIZE; i++) {
      if (!arr[i].hasres) {
        continue;
      }
      uint32 tmpid = arr[i].data.id;
      if (!update_id) {
        update_id = true;
        idx = i;
        id = tmpid;
        continue;
      }
      if (qnode.qtype == LOG_QUERY_ASC_TYPE) {
        if (tmpid < id) {
          idx = i;
          id = tmpid;
        }
      } else {
        if (tmpid > id) {
          idx = i;
          id = tmpid;
        }
      }
    }

    //写buff
    if (arr[idx].data.node.length <= LOG_RELEASE_MAX_LEN) {
      FILE *fpfile = arr[idx].fpfile[arr[idx].file_idx];
      fseek(fpfile, arr[idx].file_offset + 5, SEEK_SET);
      if (fread(buffer[added], arr[idx].data.node.length, 1, fpfile) != 1) {
        DLOG_WARNING(MOD_EB, "fread data to buffer error\n");
        break;
      }
      added++;
      qnode.last_id = arr[idx].data.id;
    }
    //获取下一个
    if (!log_LoadNextRecord(arr[idx], cur_min, cur_max, qnode.qtype)) {
      arr[idx].hasres = false;
      res_type_count--;
    }
  }

  //关闭文件
  log_CloseAllFiles(arr, LOG_TYPE_E_MAX_SIZE);
  if (!useCustomFolder) {
    LOGSVR_FILE_UNLOCK;
  }
  DLOG_INFO(MOD_EB, "Log search done, find count:%d", added);
  return added;
}

#define LOG_JSON_BODY "body"
#define LOG_JSON_TYPE_MASK "type_mask"
#define LOG_JSON_START_TIME "start_time"
#define LOG_JSON_END_TIME "end_time"
#define LOG_JSON_SEC "sec"
#define LOG_JSON_USEC "usec"
#define LOG_JSON_QNODE "qnode"
#define LOG_JSON_IS_FIRST "is_first"
#define LOG_JSON_QTYPE "qtype"
#define LOG_JSON_MIN_ID "min_id"
#define LOG_JSON_MAX_ID "max_id"
#define LOG_JSON_START_ID "start_id"
#define LOG_JSON_LAST_ID "last_id"
#define LOG_JSON_REC_SIZE "rec_size"
#define LOG_JSON_LOGS "logs"
#define LOG_JSON_TYPE "type"
#define LOG_JSON_LEVEL "level"
#define LOG_JSON_TIME "time"
#define LOG_JSON_DATA "data"
#define LOG_JSON_STATE "state"
#define LOG_JSON_ERR_MSG "err_msg"
#define LOG_JSON_LIMIT "limit"
#define LOG_JSON_MODULE "module"

std::string convert_LOG_NODE_S_type(const LOG_NODE_S &node);
std::string convert_LOG_NODE_S_level(const LOG_NODE_S &node);
std::string convert_LOG_NODE_S_time(const LOG_NODE_S &node);
bool convert_request_to_normal_data(const Json::Value &request,
                                    uint8 &type_mask, LOG_TIME_S &start,
                                    LOG_TIME_S &end, uint32 &limit,
                                    LOG_QUERY_NODE &qnode);

const int BUFFER_SIZE = 100;
int Log_SearchJson(const Json::Value &request, Json::Value &response) {
  static chml::CriticalSection crit_;
  static char buffer[BUFFER_SIZE][LOG_RELEASE_MAX_LEN];
  response.clear();
  response[LOG_JSON_TYPE] = request[LOG_JSON_TYPE];
  response[LOG_JSON_MODULE] = request[LOG_JSON_MODULE];

  chml::CritScope crit(&crit_);
  uint8 type_mask;
  uint32 limit;
  LOG_TIME_S start, end;
  LOG_QUERY_NODE qnode;
  if (!convert_request_to_normal_data(request, type_mask, start, end, limit,
                                      qnode)) {
    response[LOG_JSON_STATE] = 501;
    response[LOG_JSON_ERR_MSG] = "request json error";
    return -1;
  }

  int tot = Log_Search(type_mask, start, end, limit, buffer, qnode);
  Json::Value &rsp_body = response[LOG_JSON_BODY];

  if (tot == 0 || qnode.last_id == qnode.max_id) {
    qnode.last_id = 0;
  }
  Json::Value &rsp_qnode = rsp_body[LOG_JSON_QNODE];
  rsp_qnode[LOG_JSON_IS_FIRST] = qnode.is_first;
  rsp_qnode[LOG_JSON_QTYPE] = qnode.qtype;
  rsp_qnode[LOG_JSON_MIN_ID] = qnode.min_id;
  rsp_qnode[LOG_JSON_MAX_ID] = qnode.max_id;
  rsp_qnode[LOG_JSON_START_ID] = qnode.start_id;
  rsp_qnode[LOG_JSON_LAST_ID] = qnode.last_id;

  rsp_body[LOG_JSON_REC_SIZE] = tot;
  for (int i = 0; i < tot; i++) {
    Json::Value &data_obj = rsp_body[LOG_JSON_LOGS][i];
    LOG_NODE_S *cur = reinterpret_cast<LOG_NODE_S *>(buffer[i]);
    data_obj[LOG_JSON_TYPE] = cur->type;
    data_obj[LOG_JSON_LEVEL] = cur->level;
    data_obj[LOG_JSON_TIME] = convert_LOG_NODE_S_time(*cur);
    data_obj[LOG_JSON_DATA] =
        static_cast<char *>(&buffer[i][sizeof(LOG_NODE_S)]);
  }
  response[LOG_JSON_STATE] = 200;
  response[LOG_JSON_ERR_MSG] = "ok";
  return 0;
}

int Log_SearchBuffer(uint32 max_buffer_size, LOG_TIME_S &start, LOG_TIME_S &end, 
                     LOG_QUERY_NODE &qnode, chml::MemBuffer::Ptr res_buffer) {
  static chml::CriticalSection crit_;
  static char buffer[BUFFER_SIZE][LOG_RELEASE_MAX_LEN];
  chml::CritScope crit(&crit_);
  int tot = Log_Search(LT_POINT | LT_SYS | LT_DEV | LT_UI, start, end, BUFFER_SIZE, buffer, qnode);
  if (tot == 0 || qnode.last_id == qnode.min_id || qnode.last_id == qnode.max_id) {
    qnode.last_id = 0;
  }
  for (int i = 0; i < tot; ++i) {
    if (res_buffer->size() + LOG_RELEASE_MAX_LEN < max_buffer_size) {
      LOG_NODE_S *cur = reinterpret_cast<LOG_NODE_S *>(buffer[i]);
      res_buffer->WriteString(convert_LOG_NODE_S_time(*cur) + " "
                              + static_cast<char *>(&buffer[i][sizeof(LOG_NODE_S)]) + '\n');
    } else {
      qnode.last_id = 0;
      return 0;
    }
  }
  return 0;
}

bool convert_request_to_normal_data(const Json::Value &request,
                                    uint8 &type_mask, LOG_TIME_S &start,
                                    LOG_TIME_S &end, uint32 &limit,
                                    LOG_QUERY_NODE &qnode) {
  const Json::Value &req_body = request[LOG_JSON_BODY];

  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_TYPE_MASK].isNull(), false);
  type_mask = req_body[LOG_JSON_TYPE_MASK].asUInt();

  limit = req_body[LOG_JSON_LIMIT].asUInt();
  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_LIMIT].isNull(), false);

  limit = ML_MAX(limit, BUFFER_SIZE);

  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_START_TIME][LOG_JSON_SEC].isNull(),
                        false);
  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_START_TIME][LOG_JSON_USEC].isNull(),
                        false);
  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_END_TIME][LOG_JSON_SEC].isNull(),
                        false);
  ASSERT_RETURN_FAILURE(req_body[LOG_JSON_END_TIME][LOG_JSON_USEC].isNull(),
                        false);
  start.sec = req_body[LOG_JSON_START_TIME][LOG_JSON_SEC].asUInt();
  start.usec = req_body[LOG_JSON_START_TIME][LOG_JSON_USEC].asUInt();
  end.sec = req_body[LOG_JSON_END_TIME][LOG_JSON_SEC].asUInt();
  end.usec = req_body[LOG_JSON_END_TIME][LOG_JSON_USEC].asUInt();

  const Json::Value &req_qnode = req_body[LOG_JSON_QNODE];

  ASSERT_RETURN_FAILURE(req_qnode[LOG_JSON_QTYPE].isNull(), false);
  if (req_qnode[LOG_JSON_QTYPE].asBool()) {
    qnode.qtype = LOG_QUERY_DESC_TYPE;
  } else {
    qnode.qtype = LOG_QUERY_ASC_TYPE;
  }

  ASSERT_RETURN_FAILURE(req_qnode[LOG_JSON_IS_FIRST].isNull(), false);
  qnode.is_first = req_qnode[LOG_JSON_IS_FIRST].asBool();
  if (!qnode.is_first) {
    ASSERT_RETURN_FAILURE(req_qnode[LOG_JSON_MIN_ID].isNull(), false);
    ASSERT_RETURN_FAILURE(req_qnode[LOG_JSON_MAX_ID].isNull(), false);
    ASSERT_RETURN_FAILURE(req_qnode[LOG_JSON_START_ID].isNull(), false);
    qnode.min_id = req_qnode[LOG_JSON_MIN_ID].asInt();
    qnode.max_id = req_qnode[LOG_JSON_MAX_ID].asInt();
    qnode.start_id = req_qnode[LOG_JSON_START_ID].asInt();
  }
  return true;
}

std::string convert_LOG_NODE_S_time(const LOG_NODE_S &node) {
  chml::TimeS time;
  chml::XTimeUtil::TimeLocalMake(&time, node.sec);
  char time_str[100];
  snprintf(time_str, 100, "%4d-%02d-%02d %02d:%02d:%02d", time.year, time.month,
           time.day, time.hour, time.min, time.sec);
  return time_str;
}

std::string convert_LOG_NODE_S_type(const LOG_NODE_S &node) {
  switch ((LOG_TYPE_E_ITEM)node.type) {
    case LT_DEBUG:
      return "DEBUG";
    case LT_POINT:
      return "POINT";
    case LT_SYS:
      return "SYSTEM";
    case LT_DEV:
      return "DEV";
    case LT_UI:
      return "UI";
    case LT_USER1:
      return "USER1";
    case LT_USER2:
      return "USER2";
    case LT_USER3:
      return "USER3";
    default:
      return "UNKNOW TYPE";
  }
}
std::string convert_LOG_NODE_S_level(const LOG_NODE_S &node) {
  switch ((LOG_LEVEL_E)node.level) {
    case LL_DEBUG:
      return "DEBUG";
    case LL_INFO:
      return "INFO";
    case LL_WARNING:
      return "WARNING";
    case LL_ERROR:
      return "ERROR";
    case LL_KEY:
      return "KEY";
    case LL_NONE:
      return "NONE";
    default:
      return "UNKNOW LEVEL";
  }
}
