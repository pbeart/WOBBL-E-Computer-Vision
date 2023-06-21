#include <stdio.h>

FILE* ser;

#ifndef comm_log_def
	#define comm_log(...) {fprintf(ser, __VA_ARGS__); /*printf(__VA_ARGS__);*/}
#endif


