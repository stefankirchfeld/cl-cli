#pragma once
#include <stddef.h>

#define CL_MAX_CONTEXT_CMDS 32
#define CL_MAX_STR 512

typedef struct {
    char api_key[CL_MAX_STR];
    char model[CL_MAX_STR];
    char context_commands[CL_MAX_CONTEXT_CMDS][CL_MAX_STR];
    int  context_commands_count;
} Config;

/* Returns heap-allocated Config or NULL on fatal error. Caller frees. */
Config *config_load(void);
