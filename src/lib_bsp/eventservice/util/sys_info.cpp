#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include "sys_info.h"
#define MAX_USR_APP_CNT 32
 #include <time.h>

#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

 #include <unistd.h>
pid_t usrapp_pids[MAX_USR_APP_CNT];

static bool IsNum(const char *str, int *pid) {
  int i = 0;
  int num = 0;

  *pid = 0;
  if (str == NULL) {
    return false;
  }

  if (str[0] == '0') {
    return false;
  }
  // /num = (int)(str[i] - 'a');

  for (i = 0; i < strlen(str); i++) {
    if (isdigit(str[i]) == 0) {
      return false;
    }
    num *= 10;
    num += (int)(str[i] - '0');
  }
  *pid = num;
  return true;
}

static const char *GetUsrName(int pid, char *buf, size_t len) {
  int       ret = 0;
  ssize_t   rret = 0;
  char      line[512];
  FILE      *fp = NULL;
  size_t    seg_len = 0;
  char      *pstr = NULL;
  char      proc_name[256];

  memset(buf, 0, len);
  snprintf(proc_name, 256, "/proc/%d/environ", pid);

  fp = fopen(proc_name, "r");
  if(fp == NULL) {
    return buf;
  }
  rret = fread(line, 1, sizeof(line) - 1, fp);
  fclose(fp);

  if (rret <= 0) {
    return buf;
  }

  line[rret] = '\0';
  pstr = line;
  do {
    ret = sscanf(pstr, "USER=%s", buf);
    if (ret == 1) {
      break;
    }
    seg_len = strlen(pstr);
    pstr += seg_len + 1;
    if(pstr >= (line + rret)) {
        break;
    }
  } while(seg_len > 0);

  return (const char *)buf;
}

static uint32_t GetRss(int pid) {
  int i = 0;
  int ret = 0;
  uint32_t rss = 0;
  FILE *fp = NULL;
  char line[256];
  char item[128];
  char proc_name[256];

  snprintf(proc_name, 256, "/proc/%d/status", pid);

  fp = fopen(proc_name, "r");
  if(fp == NULL) {
    printf("open %s failed\n", proc_name);
    return 0;
  }
  //VmRSS:      3864 kB
  while (fgets(line, 256, fp) != NULL) {
    if(strstr(line, "VmRSS") == NULL) {
      continue;
    }
    for (i = 0; i < strlen(line); i++) {
      if(isdigit(line[i]) != 0) {
        rss *= 10;
        rss += (uint32_t)(line[i] - '0');
      } else {
        if(rss > 0) {
          fclose(fp);
          //printf("%d usred %u\n", pid, rss);
          return rss;
        }
      }
    }
    break;
  }
  fclose(fp);
  return 0;
}

int SearchUsrApp (const char *path) {
  int num = 0;
  int pid = 0;
  struct stat sb;
  DIR *dir = NULL;
  int pid_cnt = 0;
  char usrname[128];
  char *bname = NULL;
  const char *pusrname = NULL;
  struct dirent *file = NULL;

  dir = opendir(path);
  if(dir == NULL) {
    printf("error opendir %s!!!/n", path);
    return -1;
  }

  do {
    file = readdir(dir);
    if(file == NULL) {
      break;
    }

    bname = basename(file->d_name);
    if (IsNum(bname, &pid) == false) {
      continue;
    }

    pusrname = GetUsrName(pid, usrname, sizeof(usrname) - 1);
    if (strcmp(pusrname, "admin") != 0) {
      continue;
    }

    usrapp_pids[num++] = pid;
    if (num == MAX_USR_APP_CNT) {
      break;
    }
  } while(file != NULL);
  closedir(dir);
  return num;
}


uint32_t UsrAppMemUsed() {
  int i = 0;
  int total_rss = 0;
  int usrapp_cnt = 0;

  usrapp_cnt = SearchUsrApp ("/proc");
  if (usrapp_cnt <= 0) {
    return 0;
  }

  for (i = 0; i < usrapp_cnt; i++) {
    total_rss += GetRss(usrapp_pids[i]);
  }
  return total_rss;
}

static unsigned long GetCachedMemSize(void) {
  FILE *fp = NULL;
  char buf[200] = {0}; /* actual lines we expect are ~30 chars or less */
  unsigned long cached = 0;

  fp = fopen("/proc/meminfo", "r");
  if (fp == NULL) {
    return 0;
  }
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (sscanf(buf, "Cached: %lu", &cached) == 1) {
      break;
    }
  }
  fclose(fp);

  return cached;
}

bool GetMemInfo(meminfo_t *meminfo) {
  struct sysinfo info;
  memset(meminfo, 0, sizeof(meminfo_t));
  if (sysinfo(&info) == -1) {
    return false;
  }
  meminfo->ts = time(NULL);
  meminfo->freed = (uint32_t)info.freeram;
  meminfo->cached = (uint32_t)GetCachedMemSize();
  return true;
}
#if 0

int main(int argc, char **argv) {
    meminfo_t meminfo;
   GetMemInfo(&meminfo) ;
   printf("%d\n", meminfo.cached);
   return 0;
} 
#endif