#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>

#define STACK_DEPS 6

void DumpLine(const char *line, const char *exec_file) {
  if (line[0] == '=') {
    printf("%s\n", line);
    return;
  }
  std::string type;
  uint64_t ptr;
  uint32_t size;
  uint64_t p[STACK_DEPS];
  const char *fmt;
  if (strncmp("ALLOC", line, 5) == 0) {
    type = "MEM LEAK";
    fmt = "ALLOC %llx %u %llx %llx %llx %llx %llx %llx";
  } else if (strncmp("FREE", line, 4) == 0) {
    type = "FREE";
    fmt = "FREE  %llx %u %llx %llx %llx %llx %llx %llx";
  } else {
    //if(strlen(line))
    //printf("error type:%s\n", line);
    return;
  }

  int ret = sscanf(line, fmt,
                   &ptr, &size, &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]);
  if (ret != 8) {
    printf("error line:%s\n", line);
    return;
  }

  printf("====MEM %s, size %u, addr %llx\n", type.c_str(), size, ptr);
  for (int i = 0; i < STACK_DEPS; i++) {
    if (p[i] != 0) {
      char cmd[128];
      sprintf(cmd, "addr2line -p -s -e %s -f 0x%llx", exec_file, p[i]);
      system(cmd);
    }
  }
}

void ReadLog(const char *exec_file, const char *log_file) {
  std::fstream fs;
  fs.open(log_file, std::fstream::in);
  if (!fs.is_open()) {
    printf("open %s failed\n", log_file);
    return;
  }
  char line[256];
  while (!fs.eof()) {
    fs.getline(line, 256);
    DumpLine(line, exec_file);
  }
  fs.close();
}

int main(int argc, char **argv) {
  if(argc != 3) {
    printf("%s execfile logfile\n", argv[0]);
    return -1;
  }
  ReadLog(argv[1], argv[2]);
  return 0;
}