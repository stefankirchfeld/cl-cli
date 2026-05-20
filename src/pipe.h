#pragma once
#include "config.h"

/* Read all of stdin, call Claude with context + question, print to stdout.
   freetext=1 allows FREETEXT responses (-f flag).
   Returns 0 on success, 1 on error. */
int pipe_mode(const Config *config, const char *context, const char *question,
              int freetext);
