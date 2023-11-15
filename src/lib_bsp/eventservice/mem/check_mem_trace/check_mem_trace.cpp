/**/ /*mtrace.c*/
#include <stdio.h>
#include <string.h>

#define MAX_ENTRY 1024

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("usage: %s [log file] [out file] ", argv[0]);
    return 0;
  }

  FILE* fp = fopen(argv[1], "r");
  FILE* fp_out = fopen(argv[2], "wb+");

  if (fp == NULL || fp_out == NULL) {
    printf("open file failed ");
    if (fp != NULL) {
      fclose(fp);
    }
    if (fp_out != NULL) {
      fclose(fp_out);
    }
    return -1;
  }

  int i = 0;
  int n = 0;
  int skip = 0;
  int line_index = 0;
  void* addr = 0;
  char line[260] = {0};
  void* addrs_array[MAX_ENTRY] = {0};
  int lines_array[MAX_ENTRY] = {0};

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (line[0] != 'a' && line[0] != 'f') {
      line_index++;
      continue;
    }

    addr = NULL;
    if (strncmp(line, "alloc", 5) == 0 && n < MAX_ENTRY) {
      sscanf(line, "alloc %p", &addr);
      addrs_array[n] = addr;
      lines_array[n] = line_index;
      n++;

      printf("a\n");
    } else if (strncmp(line, "free", 4) == 0) {
      sscanf(line, "free %p", &addr);
      for (i = 0; i < n; i++) {
        if (addrs_array[i] == addr) {
          lines_array[i] = -1;
          break;
        }
      }

      printf("f\n");
    }
    line_index++;
  }

  printf(" ");
  fseek(fp, 0, 0);

  i = 0;
  line_index = 0;
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (strncmp(line, "alloc", 5) == 0) {
      if (lines_array[i] == line_index) {
        printf("leak %s", line);
        fprintf(fp_out, "*");
        skip = 0;
        i++;
      } else {
        skip = 1;
      }
    } else if (strncmp(line, "free", 4) == 0) {
      skip = 1;
    }

    if (!skip) {
      fputs(line, fp_out);
    }

    line_index++;
  }

  fclose(fp);
  fclose(fp_out);
  return 0;
}