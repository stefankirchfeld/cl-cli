#pragma once
#include "config.h"

/* Runs all context_commands from config, returns heap-allocated string
   with combined output. Caller frees. Never returns NULL (may be empty). */
char *context_collect(const Config *config);
