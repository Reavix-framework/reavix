#include <uv.h>
#include <string.h>
#include "router.h"

void log_error(const char* msg){
    fprintf(stderr, "Error: %s\n", msg);
}