#include "kvdboperatefile.h"
#include "eventservice/log/log_client.h"
#include "eventservice/util/file_util.h"
#include "eventservice/util/file_opera.h"
#include "kvdbclient.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#ifdef WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace cache {

/** 路径缓存长度 */
#define KVDB_TMP_BUFFER_LEN      1024

KvdbOperateFile::KvdbOperateFile(const char *foldpath, int property)
  : property_(property) {
  if (foldpath == NULL || strlen(foldpath) > KVDB_FILE_PATH_MAXLEN) {
#ifdef WIN32
    const char *def_folder = "D:\\kvdb";
#elif defined LITEOS
    const char *def_folder = "/usr/kvdb";
#else
    const char *def_folder = "/mnt/usr/kvdb";
#endif
    strcpy(foldpath_, def_folder);
    DLOG_WARNING(MOD_EB, "Invalid foldpath %p or exceed max length %d,"
                "use the default path %s",
                foldpath, KVDB_FILE_PATH_MAXLEN, def_folder);
  } else {
    strcpy(foldpath_, foldpath);
  }
  // 创建文件夹
  FoldCreate(foldpath_);
  kvdb_check_ = new KvdbParamCheck();
  if (!kvdb_check_) {
    DLOG_ERROR(MOD_EB, "new kvdb_check_ faild !");
  }
}

KvdbOperateFile::~KvdbOperateFile() {
  if (kvdb_check_) {
    delete kvdb_check_;
  }  
}

void KvdbOperateFile::GetWholePath(char *foldpath, int32 maxlen, 
                              const char *parentfold, const char *endpath,
                              bool backupflag) {
#ifdef WIN32
  if (backupflag) {
    snprintf(foldpath, maxlen, "%s\\%s_bak", parentfold, endpath);
  }
  else {
    snprintf(foldpath, maxlen, "%s\\%s", parentfold, endpath);
  }
#else
  if (backupflag) {
    snprintf(foldpath, maxlen, "%s/%s_bak", parentfold, endpath);
  }
  else {
    snprintf(foldpath, maxlen, "%s/%s", parentfold, endpath);
  }
#endif
}

KvdbError KvdbOperateFile::WriteKey(const std::string &key, const char *value, int length) {
  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    DLOG_ERROR(MOD_EB, "Replace error key");
    return KVDB_ERR_INVALID_PARAM;
  }

  chml::CritScope cr(&crit_);
  // 操作文件
  std::string match_value;
  const std::string match_key = key + "_match";
  KvdbError readret = ReadFile(match_key, match_value, false);

  if (readret == KVDB_ERR_OK && kvdb_check_) {
    bool checkret = kvdb_check_->check_param_valid(std::string(value, length), match_value);
    if (!checkret) {
      DLOG_ERROR(MOD_EB, "Replace result %d", KVDB_ERR_FILE_CHECK);
      return KVDB_ERR_FILE_CHECK;
    }
  } else {
    DLOG_ERROR(MOD_EB, "can't read %s match file.", key.c_str());
  }

  KvdbError ret = WriteFile(key, (uint8*)value, length, false);
  if (property_ & KVDB_SAFE_MODE) {
    ret = WriteFile(key, (uint8*)value, length, true);
    if (ret != KVDB_ERR_OK) {
      DLOG_ERROR(MOD_EB, "Replace result %d key : %s", ret, key.c_str());
      return ret;
    }
  }
  if (ret != KVDB_ERR_OK) {
    DLOG_ERROR(MOD_EB, "Replace result %d key : %s", ret, key.c_str());
    return ret;
  }

  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::WriteKey(const std::string &key, const std::string &value) {
  return WriteKey(key, value.c_str(), value.length());
}

KvdbError KvdbOperateFile::Remove(const std::string &key) {
  // 准备文件路径
  char filename_temp[KVDB_FILE_PATH_MAXLEN + 5];
  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    DLOG_ERROR(MOD_EB, "Remove error key");
    return KVDB_ERR_INVALID_PARAM;
  }

  chml::CritScope cr(&crit_);
  // 操作文件
  GetWholePath(filename_temp, KVDB_FILE_PATH_MAXLEN + 5, foldpath_, key.c_str(), false);
  if (::remove(filename_temp) != 0) {
    DLOG_ERROR(MOD_EB, "Remove result %d key : %s", KVDB_ERR_WRITE_FILE, key.c_str());
    return KVDB_ERR_WRITE_FILE;
  }

  // 操作文件
  GetWholePath(filename_temp, KVDB_FILE_PATH_MAXLEN + 5, foldpath_, key.c_str(), true);
  if (::remove(filename_temp) != 0) {
    //备份文件可能删除失败
  }
  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::Clear(const char *target_fold) {
  char buf[KVDB_TMP_BUFFER_LEN] = {0};
  KvdbError rtn = KVDB_ERR_OK;
  if (target_fold == NULL) {
    target_fold = foldpath_;
  }
#ifdef WIN32
  WIN32_FIND_DATA FindFileData;

  strcpy(buf, target_fold);
  strcat(buf, "\\*.*");
  HANDLE hFind = FindFirstFile(buf, &FindFileData);
  if (INVALID_HANDLE_VALUE == hFind)
    return rtn;

  do {
    if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      ::remove(FindFileData.cFileName);
    }
  } while (FindNextFile(hFind, &FindFileData));

  FindClose(hFind);

#else
  DIR *dir;
  struct stat statbuf;
  struct dirent *dirent;

  // 清空文件
  dir = opendir(target_fold);
  if (dir == NULL) {
    return KVDB_ERR_OPEN_FOLD;
  }
  while ((dirent = readdir(dir)) != NULL) {
    snprintf(buf, KVDB_TMP_BUFFER_LEN, "%s/%s", target_fold, dirent->d_name);
    if (stat(buf, &statbuf) == -1) {
      DLOG_ERROR(MOD_EB, "Stat file error buf : %s", buf);
      rtn = KVDB_ERR_DELETE_FILE;
      continue;
    }
    if (S_ISREG(statbuf.st_mode)) {
      if (::remove(buf) != 0) {
        DLOG_ERROR(MOD_EB, "Delete file error buf : %s", buf);
        rtn = KVDB_ERR_DELETE_FILE;
      }
    }
  }
  closedir(dir);
#endif
  return rtn;
}

KvdbError KvdbOperateFile::ReadKey(const std::string &key, std::string &get_value) {
  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    DLOG_ERROR(MOD_EB, "Error key");
    return KVDB_ERR_INVALID_PARAM;
  }

  chml::CritScope cr(&crit_);
  get_value.clear();

  std::string get_data;
  const std::string defkey = key + "_match";
  KvdbError readret = ReadFile(defkey, get_data, false);
  std::string sdefault = "";
  if (kvdb_check_) {
    sdefault = kvdb_check_->get_default_param(get_data);
  }

  KvdbError ret = ReadFile(key, get_value, false);
  if (ret != KVDB_ERR_OK) {
    if (property_ & KVDB_SAFE_MODE) {
      ret = ReadFile(key, get_value, true);
      if (ret != KVDB_ERR_OK) {
        if (readret == KVDB_ERR_OK && sdefault != "") {
          get_value = sdefault;
        } else {
          DLOG_ERROR(MOD_EB, "result %d key : %s", ret, key.c_str());
          return ret;
        }
      };
    } else {
      if (readret == KVDB_ERR_OK && sdefault != "") {
        get_value = sdefault;
      } else {
        DLOG_ERROR(MOD_EB, "result %d key : %s", ret, key.c_str());
        return ret;
      }
    }
  }

  if (readret == KVDB_ERR_OK && sdefault != "" && kvdb_check_) {
    bool checkret = kvdb_check_->check_param_valid(get_value, get_data);
    if (!checkret) {
      get_value = sdefault;
    }
  }

  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::Backup(const char *backup_fold, const char *name) {
  if (backup_fold == NULL || name == NULL) {
    DLOG_ERROR(MOD_EB, "Error: backup fold or name == NULL! ");
    return KVDB_ERR_INVALID_PARAM;
  }
  char buf[KVDB_TMP_BUFFER_LEN] = {0};
  GetWholePath(buf, KVDB_TMP_BUFFER_LEN, foldpath_, name, false);
  return FoldCopy(backup_fold, buf);
}

KvdbError KvdbOperateFile::Restore(const char *backup_fold, const char *name) {
  if (backup_fold == NULL || name == NULL) {
    DLOG_ERROR(MOD_EB, "Error: restore fold or name == NULL! ");
    return KVDB_ERR_INVALID_PARAM;
  }
  char buf[KVDB_TMP_BUFFER_LEN] = {0};
  GetWholePath(buf, KVDB_TMP_BUFFER_LEN, foldpath_, name, false);
  return FoldCopy(buf, backup_fold);
}

KvdbError KvdbOperateFile::GetProperty(int &property) {
  property = property_;
  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::SetProperty(int property) {
  property_ = property;
  return KVDB_ERR_OK;
}

////////////////////////////////////////////////////////////////////////////////
KvdbError KvdbOperateFile::WriteFile(const std::string &key, uint8 *buffer, int length, bool isbak) {
  // 准备文件路径
  char buf[KVDB_TMP_BUFFER_LEN + 5] = {0};
  if (isbak) {
    GetWholePath(buf, KVDB_TMP_BUFFER_LEN+5, foldpath_, key.c_str(), true);
  } else {
    GetWholePath(buf, KVDB_TMP_BUFFER_LEN+5, foldpath_, key.c_str(), false);
  }

  // 快速重复打开文件，warning打印提醒
  struct stat statbuf;
  if (stat(buf, &statbuf) == -1) {  // 获取文件状态
    DLOG_WARNING(MOD_EB, "Stat file error, maybe file is not exist");    
  }
  else {
    if (time(NULL) - statbuf.st_mtime < 1) {
      DLOG_WARNING(MOD_EB, "%s write file too fast !", key.c_str());
    }
  }

  chml::XFile kvdb_file_;
  bool filestatus = kvdb_file_.open(buf, chml::IO_MODE_REWR_ORNEW);
  if (!filestatus) {
    DLOG_WARNING(MOD_EB, "open file fail!");
    return KVDB_ERR_OPEN_FILE;
  }

  uint32 flag = KVDB_FILE_HEAD_FLAG;
  int32 w_len = kvdb_file_.write_data((char *)&flag, sizeof(flag), 1);
  if (w_len != sizeof(flag)) {
    kvdb_file_.close();
    DLOG_WARNING(MOD_EB, "w_len != sizeof(flag)");
    return KVDB_ERR_WRITE_FILE;
  }
  w_len = kvdb_file_.write_data((char *)&length, sizeof(length), 1);
  if (w_len != sizeof(length)) {
    kvdb_file_.close();
    DLOG_WARNING(MOD_EB, "w_len != sizeof(length)");
    return KVDB_ERR_WRITE_FILE;
  }
  uint8 checksum = 0;
  //计算校验和
  for (int i = 0; i < 4; i++) {
    checksum ^= (length >> (i * 8));
  }
  for (int i = 0; i < length; i++) {
    checksum ^= buffer[i];
  }
  w_len = kvdb_file_.write_data((char *)&checksum, sizeof(checksum), 1);
  if (w_len != sizeof(checksum)) {
    kvdb_file_.close();
    DLOG_WARNING(MOD_EB, "w_len != sizeof(checksum)");
    return KVDB_ERR_WRITE_FILE;
  }
  w_len = kvdb_file_.write_data((const char*)buffer, length, 1);
  if ((int) w_len != length) {
    kvdb_file_.close();
    DLOG_WARNING(MOD_EB, "w_len != length");
    return KVDB_ERR_WRITE_FILE;
  }
  kvdb_file_.close();
  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::ReadFile(const std::string &key, std::string &get_data, bool isbak) {
  // 准备文件路径
  char buf[KVDB_TMP_BUFFER_LEN + 1] = {0};
  if (isbak) {
    GetWholePath(buf, KVDB_TMP_BUFFER_LEN+5, foldpath_, key.c_str(), true);
  } else {
    GetWholePath(buf, KVDB_TMP_BUFFER_LEN+5, foldpath_, key.c_str(), false);
  }

  if (!chml::XFileUtil::is_exist((char*)buf)) {
    DLOG_WARNING(MOD_EB, "%s fail is not exist !", key.c_str());
    return KVDB_ERR_FILE_DONTEXIST;
  }

  chml::XFile kvdb_file_;
  bool filestatus = kvdb_file_.open(buf, chml::IO_MODE_RD_ONLY);
  if (!filestatus) {
    DLOG_WARNING(MOD_EB, "%s open file fail!", key.c_str());
    return KVDB_ERR_FILE_DONTEXIST;
  }

  int len;
  uint32 flag;
  uint8 checksum;
  kvdb_file_.read_data((char *)&flag, sizeof(flag), 1);
  if (flag != KVDB_FILE_HEAD_FLAG) {
    if (isbak) {
      kvdb_file_.close();
      return KVDB_ERR_FILE_CHECK;
    }
    //暂时兼容以前的配置文件，一段时间后得取消兼容
    kvdb_file_.set_pos(0);
    int rdsize = 0, len;
    while (1) {
      len = kvdb_file_.read_data(buf, KVDB_TMP_BUFFER_LEN, 1);
      buf[len] = '\0';
      get_data.append((char *)buf, len);
      rdsize += len;
      if (len == 0) {
        break;
      }
    }
    kvdb_file_.close();
    int32 ret = WriteFile(key, (uint8 *)get_data.c_str(), get_data.size(), false);
    if (ret != KVDB_ERR_OK) {
      DLOG_ERROR(MOD_EB, "ReadFile WriteFile result %d key : %s", ret, key.c_str());
    }
    return ReadFile(key, get_data, isbak);
  }
  kvdb_file_.read_data((char *)&len, sizeof(len), 1);
  kvdb_file_.read_data((char *)&checksum, sizeof(checksum), 1);

  // 读文件
  int rdlen = 0;
  get_data.clear();
  while (rdlen < len) {
    int cur = len - rdlen;
    cur = cur > KVDB_TMP_BUFFER_LEN ? KVDB_TMP_BUFFER_LEN : cur;
    int rd = kvdb_file_.read_data(buf, cur, 1);
    if (rd == 0) {
      break;
    }
    buf[rd] = '\0';
    get_data.append((char *)buf, rd);
    rdlen += rd;
  }
  kvdb_file_.close();
  //校验
  if (property_ & KVDB_SAFE_MODE) {
    uint8 tmp_sum = 0;
    for (int i = 0; i < 4; i++) {
      tmp_sum ^= (len >> (i * 8));
    }
    if (rdlen != len) {
      return KVDB_ERR_FILE_CHECK;
    }
    for (int i = 0; i < len; i++) {
      tmp_sum ^= get_data[i];
    }
    if (tmp_sum != checksum) {
      return KVDB_ERR_FILE_CHECK;
    }
  }

  return KVDB_ERR_OK;
}

KvdbError KvdbOperateFile::FoldCreate(const char *fold_path) {
  int len;
  char buf[KVDB_TMP_BUFFER_LEN] = {0};

  if (fold_path == NULL || strlen(fold_path) == 0) {
    return KVDB_ERR_INVALID_PARAM;
  }
  strncpy(buf, fold_path, 512);
  len = strlen(fold_path);

  bool ret = chml::XFileUtil::make_dir_p(std::string(buf, len));
  if (ret) {
    return KVDB_ERR_OK;
  }
  else {
    DLOG_WARNING(MOD_EB, "make_dir_p fail!");
    return KVDB_ERR_OPEN_FILE;
  }
}

KvdbError KvdbOperateFile::FoldCopy(const char *target_fold, const char *src_fold) {
  char buf_src[KVDB_TMP_BUFFER_LEN];
  char buf_target[KVDB_TMP_BUFFER_LEN];
  KvdbError rtn = KVDB_ERR_OK;

  chml::CritScope cr(&crit_);
  // chml::XFileUtil::make_dir_p(std::string(target_fold));
#ifdef WIN32
  FoldCreate(target_fold);  // 创建文件夹
  Clear(target_fold);  // 清空目标文件夹

  WIN32_FIND_DATA FindFileData;

  strcpy(buf_src, src_fold);
  strcat(buf_src, "\\*.*");
  HANDLE hFind = FindFirstFile(buf_src, &FindFileData);
  if (INVALID_HANDLE_VALUE == hFind)
    return rtn;
  do {
    if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      snprintf(buf_src, KVDB_TMP_BUFFER_LEN, "%s\\%s", src_fold, FindFileData.cFileName);
      snprintf(buf_target, KVDB_TMP_BUFFER_LEN, "%s\\%s", target_fold, FindFileData.cFileName);
      rtn = FileCopy(buf_target, buf_src);  // 拷贝文件
      if (rtn != KVDB_ERR_OK)
        break;
    }
  } while (FindNextFile(hFind, &FindFileData));

  FindClose(hFind);
#else
  DIR *dir;
  struct stat statbuf;
  struct dirent *dirent;

  FoldCreate(target_fold);  // 创建文件夹
  Clear(target_fold);  // 清空目标文件夹
  dir = opendir(src_fold);
  if (dir == NULL) {
    return KVDB_ERR_OPEN_FILE;
  }
  while ((dirent = readdir(dir)) != NULL) {  // 遍历文件夹
    snprintf(buf_src, KVDB_TMP_BUFFER_LEN, "%s/%s", src_fold, dirent->d_name);
    if (stat(buf_src, &statbuf) == -1) {  // 获取文件状态
      DLOG_ERROR(MOD_EB, "Stat file error");
      rtn = KVDB_ERR_DELETE_FILE;
      continue;
    }
    if (S_ISREG(statbuf.st_mode)) {  // 普通文件
      snprintf(buf_target, KVDB_TMP_BUFFER_LEN, "%s/%s", target_fold, dirent->d_name);
      rtn = FileCopy(buf_target, buf_src);  // 拷贝文件
      if (rtn != KVDB_ERR_OK)
        break;
    }
  }
  closedir(dir);
#endif
  return rtn;
}

KvdbError KvdbOperateFile::FileCopy(const char *target_filename, const char *src_filename) {
  FILE *srcfile;
  FILE *targetfile;
  int c;
  KvdbError rtn = KVDB_ERR_OK;

  if (src_filename == NULL || target_filename == NULL) {
    return KVDB_ERR_INVALID_PARAM;
  }
  srcfile = fopen(src_filename, "rb");
  if (srcfile == NULL) {
    return KVDB_ERR_OPEN_FILE;
  }
  targetfile = fopen(target_filename, "wb");
  if (targetfile == NULL) {
    fclose(srcfile);
    return KVDB_ERR_OPEN_FILE;
  }
  for (;;) {
    c = fgetc(srcfile);
    if (feof(srcfile)) {
      break;
    }
    fputc(c, targetfile);
  }
  fclose(srcfile);
  fclose(targetfile);
  return rtn;
}

}
