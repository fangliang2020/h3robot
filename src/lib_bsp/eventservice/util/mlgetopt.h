#ifndef EVENTSERVICE_UTIL_MLGETOPT_H
#define EVENTSERVICE_UTIL_MLGETOPT_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

extern int ml_optopt;
extern char* ml_optarg;
int ml_getopt(int argc, char** argv, char* opts);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif  // !EVENTSERVICE_UTIL_MLGETOPT_H


