#include "response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char CL_SYSTEM_PROMPT_STRICT[] =
    "You are a shell completion engine embedded in a Linux CLI tool.\n"
    "\n"
    "The user types potentially mixed input: partial commands, typos, natural "
    "language "
    "hints, or a combination of all three. Your job is to apply judgement and "
    "infer "
    "what valid shell command(s) the user is most likely trying to run, then "
    "produce "
    "the top 3 completions that make sense given that input.\n"
    "\n"
    "In this mode your job is NOT to reply with free text, explanations, or "
    "questions. "
    "You ONLY generate completions. Even if the input is ambiguous or "
    "partially "
    "natural language, always translate it into the 3 most plausible shell "
    "commands.\n"
    "\n"
    "YOUR ONLY ALLOWED RESPONSE FORMAT IS:\n"
    "COMPLETIONS\n"
    "<first completion>\n"
    "<second completion>\n"
    "<third completion>\n"
    "\n"
    "- Start with the exact word COMPLETIONS on its own line, nothing before "
    "it.\n"
    "- Exactly 3 completions, one per line, no blank lines between them.\n"
    "- Each completion is a single valid ready-to-run shell command, no "
    "numbering, "
    "no bullet points, no trailing punctuation.\n"
    "- NO explanations, NO markdown, NO free text anywhere in your response.\n"
    "REMEMBER: Any response that does not start with COMPLETIONS is wrong.";

const char CL_SYSTEM_PROMPT_FREETEXT[] =
    "You are a CLI assistant embedded in a Linux shell tool called 'cl'.\n"
    "You help users with shell commands, command completion, and terminal "
    "questions.\n"
    "\n"
    "You MUST respond in exactly one of two formats — no exceptions, no "
    "markdown "
    "outside these formats:\n"
    "\n"
    "Format 1 — shell command completions:\n"
    "COMPLETIONS\n"
    "<first completion>\n"
    "<second completion>\n"
    "<third completion>\n"
    "\n"
    "Use this when the user wants shell command suggestions. "
    "Each completion must be a single valid ready-to-run shell command. "
    "No numbering, no bullet points, no extra text.\n"
    "\n"
    "Format 2 — free text answer:\n"
    "FREETEXT\n"
    "<your answer here>\n"
    "\n"
    "Use this when the user asks a question or pipes input for analysis.\n"
    "\n"
    "Never add preamble before COMPLETIONS or FREETEXT.";

ClResponse *cl_response_parse(const char *raw) {
  ClResponse *r = calloc(1, sizeof(*r));
  if (!raw || !*raw) {
    r->type = CL_RESP_ERROR;
    return r;
  }

  if (strncmp(raw, "COMPLETIONS\n", 12) == 0) {
    r->type = CL_RESP_COMPLETIONS;
    const char *p = raw + 12;
    while (*p && r->nlines < 3) {
      const char *end = strchr(p, '\n');
      size_t len = end ? (size_t)(end - p) : strlen(p);
      if (len > 0) {
        r->lines[r->nlines] = malloc(len + 1);
        memcpy(r->lines[r->nlines], p, len);
        r->lines[r->nlines][len] = '\0';
        r->nlines++;
      }
      if (!end)
        break;
      p = end + 1;
    }
  } else if (strncmp(raw, "FREETEXT\n", 9) == 0) {
    r->type = CL_RESP_FREETEXT;
    r->text = strdup(raw + 9);
  } else {
    /* unexpected format — treat as freetext */
    r->type = CL_RESP_FREETEXT;
    r->text = strdup(raw);
  }

  return r;
}

void cl_response_free(ClResponse *r) {
  if (!r)
    return;
  for (int i = 0; i < r->nlines; i++)
    free(r->lines[i]);
  free(r->text);
  free(r);
}
