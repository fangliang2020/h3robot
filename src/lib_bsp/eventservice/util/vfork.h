#ifndef ML_VFORK_H
#define ML_VFORK_H

#include <stdio.h>



 int vsystem(const char * cmdstring);
 int vpclose(FILE *fp);
 FILE *vpopen(const char* cmdstring, const char *type);
 


 #endif