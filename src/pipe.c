#include "pipe.h"
#include "claude.h"
#include "response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pipe_mode(const Config *config, const char *context, const char *question,
              int freetext) {
  /* read all stdin */
  size_t cap = 65536, len = 0;
  char *stdin_buf = malloc(cap);
  if (!stdin_buf)
    return 1;

  size_t n;
  while ((n = fread(stdin_buf + len, 1, cap - len - 1, stdin)) > 0) {
    len += n;
    if (len + 1 >= cap) {
      cap *= 2;
      stdin_buf = realloc(stdin_buf, cap);
      if (!stdin_buf)
        return 1;
    }
  }
  stdin_buf[len] = '\0';

  /* build user prompt */
  size_t prompt_cap = len + (context ? strlen(context) : 0) + 512;
  char *prompt = malloc(prompt_cap);
  if (!prompt) {
    free(stdin_buf);
    return 1;
  }

  int off = 0;
  if (context && context[0])
    off += snprintf(prompt + off, prompt_cap - (size_t)off, "Context:\n%s\n",
                    context);
  if (len > 0)
    off += snprintf(prompt + off, prompt_cap - (size_t)off, "Input:\n%s\n",
                    stdin_buf);
  if (question && question[0])
    snprintf(prompt + off, prompt_cap - (size_t)off, "Question: %s", question);

  free(stdin_buf);

  const char *sysprompt =
      freetext ? CL_SYSTEM_PROMPT_FREETEXT : CL_SYSTEM_PROMPT_STRICT;
  char *raw =
      claude_complete(config->api_key, config->model, sysprompt, prompt);
  free(prompt);
  if (!raw)
    return 1;

  ClResponse *resp = cl_response_parse(raw);
  free(raw);

  int rc = 0;
  if (resp->type == CL_RESP_COMPLETIONS) {
    if (resp->nlines > 0)
      fputs(resp->lines[0],
            stdout); /* no trailing newline — safe in readline */
  } else if (resp->type == CL_RESP_FREETEXT) {
    puts(resp->text);
  } else {
    fprintf(stderr, "cl: unexpected response format\n");
    rc = 1;
  }

  cl_response_free(resp);
  return rc;
}
