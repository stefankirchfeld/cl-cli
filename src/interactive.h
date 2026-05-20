#pragma once
#include "config.h"

/* Launch interactive TUI. freetext=1 allows FREETEXT responses (-f flag).
   initial may be NULL or a pre-populated input string.
   Prints chosen completion to stdout and exits.
   Returns 0 if user picked a completion, 1 if they cancelled (Ctrl-C). */
int interactive_mode(const Config *config, const char *context, int freetext,
                     const char *initial);
