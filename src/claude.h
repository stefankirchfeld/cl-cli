#pragma once

#include "config.h"

typedef enum {
  CL_RESP_COMPLETIONS,
  CL_RESP_FREETEXT,
  CL_RESP_ERROR,
} ClResponseType;

typedef struct {
  ClResponseType type;
  char* lines[3]; /* for COMPLETIONS: up to 3 heap-allocated strings */
  int nlines;
  char* text; /* for FREETEXT: heap-allocated string */
} ClResponse;

/* Call the Anthropic Messages API. Returns heap-allocated response text
   or NULL on error. Caller frees. */
char* claude_complete(const char* api_key, const char* model,
                      const char* system_prompt, const char* user_prompt);
/* Parse a raw Claude response string into a ClResponse.
   Returns heap-allocated ClResponse. Caller frees with cl_response_free(). */
ClResponse* cl_response_parse(const char* raw);
void cl_response_free(ClResponse* r);

/* System prompt for strict completions-only mode (default, no -f flag). */
extern const char CL_SYSTEM_PROMPT_STRICT[];

/* System prompt permitting both COMPLETIONS and FREETEXT (-f flag). */
extern const char CL_SYSTEM_PROMPT_FREETEXT[];

ClResponse* fetch_response(const Config* config, const char* context,
                           const char* input, int freetext);
