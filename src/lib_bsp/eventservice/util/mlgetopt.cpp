#include "mlgetopt.h"
#include <stdio.h>
#include <memory.h>
#include <string.h>

int ml_optopt;
char *ml_optarg;
int ml_opterr = 1;
int ml_optind = 1;

int ml_getopt(int argc, char **argv, char *opts) {
  static int sp = 1;
  register int c;
  register char *cp;

  if (sp == 1) {
    if (ml_optind >= argc || argv[ml_optind][0] != '-' ||
        argv[ml_optind][1] == '\0') {
      return (EOF);
    } else if (strcmp(argv[ml_optind], "--") == 0) {
      ml_optind++;
      return (EOF);
    }
  }

  ml_optopt = c = argv[ml_optind][sp];
  if (c == ':' || (cp = strchr(opts, c)) == NULL) {
    // ERR(": illegal option -- ", c);
    if (argv[ml_optind][++sp] == '\0') {
      ml_optind++;
      sp = 1;
    }
    return ('?');
  }
  if (*++cp == ':') {
    if (argv[ml_optind][sp + 1] != '\0') {
      ml_optarg = &argv[ml_optind++][sp + 1];
    } else if (++ml_optind >= argc) {
      printf("%c: option requires an argument -- \n", c);
      sp = 1;
      return ('?');
    } else {
      ml_optarg = argv[ml_optind++];
    }
    sp = 1;
  } else {
    if (argv[ml_optind][++sp] == '\0') {
      sp = 1;
      ml_optind++;
    }
    ml_optarg = NULL;
  }
  return (c);
}
