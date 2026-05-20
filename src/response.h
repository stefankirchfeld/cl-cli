#pragma once

typedef enum {
    CL_RESP_COMPLETIONS,
    CL_RESP_FREETEXT,
    CL_RESP_ERROR,
} ClResponseType;

typedef struct {
    ClResponseType type;
    char *lines[3];   /* for COMPLETIONS: up to 3 heap-allocated strings */
    int   nlines;
    char *text;       /* for FREETEXT: heap-allocated string */
} ClResponse;

/* Parse a raw Claude response string into a ClResponse.
   Returns heap-allocated ClResponse. Caller frees with cl_response_free(). */
ClResponse *cl_response_parse(const char *raw);
void        cl_response_free(ClResponse *r);

/* System prompt for strict completions-only mode (default, no -f flag). */
extern const char CL_SYSTEM_PROMPT_STRICT[];

/* System prompt permitting both COMPLETIONS and FREETEXT (-f flag). */
extern const char CL_SYSTEM_PROMPT_FREETEXT[];
