#include "claude.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat.h"
#include "config.h"

#define ANTHROPIC_URL "https://api.anthropic.com/v1/messages"
#define ANTHROPIC_VERSION "2023-06-01"
#define MAX_TOKENS 1024
#define INITIAL_BUF 65536

typedef struct {
  char* data;
  size_t len, cap;
} Buf;

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* ud) {
  Buf* b = (Buf*)ud;
  size_t n = size * nmemb;
  if (b->len + n + 1 > b->cap) {
    b->cap = (b->len + n + 1) * 2;
    b->data = realloc(b->data, b->cap);
    if (!b->data) return 0;
  }
  memcpy(b->data + b->len, ptr, n);
  b->len += n;
  b->data[b->len] = '\0';
  return n;
}

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

ClResponse* cl_response_parse(const char* raw) {
  ClResponse* r = calloc(1, sizeof(*r));
  if (!raw || !*raw) {
    r->type = CL_RESP_ERROR;
    return r;
  }

  if (strncmp(raw, "COMPLETIONS\n", 12) == 0) {
    r->type = CL_RESP_COMPLETIONS;
    const char* p = raw + 12;
    while (*p && r->nlines < 3) {
      const char* end = strchr(p, '\n');
      size_t len = end ? (size_t)(end - p) : strlen(p);
      if (len > 0) {
        r->lines[r->nlines] = malloc(len + 1);
        memcpy(r->lines[r->nlines], p, len);
        r->lines[r->nlines][len] = '\0';
        r->nlines++;
      }
      if (!end) break;
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

void cl_response_free(ClResponse* r) {
  if (!r) return;
  for (int i = 0; i < r->nlines; i++) free(r->lines[i]);
  free(r->text);
  free(r);
}

char* claude_complete(const char* api_key, const char* model,
                      const char* system_prompt, const char* user_prompt) {
  /* build request JSON */
  json_t* root = json_object();
  json_object_set_new(root, "model", json_string(model));
  json_object_set_new(root, "max_tokens", json_integer(MAX_TOKENS));

  if (system_prompt && system_prompt[0])
    json_object_set_new(root, "system", json_string(system_prompt));

  json_t* msgs = json_array();
  json_t* msg = json_object();
  json_object_set_new(msg, "role", json_string("user"));
  json_object_set_new(msg, "content", json_string(user_prompt));
  json_array_append_new(msgs, msg);
  json_object_set_new(root, "messages", msgs);

  char* body = json_dumps(root, JSON_COMPACT);
  json_decref(root);
  if (!body) return NULL;

  /* set up headers */
  char auth_header[CL_MAX_STR + 32];
  snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);

  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "anthropic-version: " ANTHROPIC_VERSION);

  /* perform request */
  Buf buf = {.data = malloc(INITIAL_BUF), .len = 0, .cap = INITIAL_BUF};
  if (!buf.data) {
    curl_slist_free_all(headers);
    free(body);
    return NULL;
  }

  CURL* curl = curl_easy_init();
  if (!curl) {
    curl_slist_free_all(headers);
    free(body);
    free(buf.data);
    return NULL;
  }
  curl_easy_setopt(curl, CURLOPT_URL, ANTHROPIC_URL);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  free(body);

  if (res != CURLE_OK) {
    fprintf(stderr, "cl: curl error: %s\n", curl_easy_strerror(res));
    free(buf.data);
    return NULL;
  }

  /* parse response */
  json_error_t err;
  json_t* resp = json_loads(buf.data, 0, &err);
  free(buf.data);

  if (!resp) {
    fprintf(stderr, "cl: JSON parse error: %s\n", err.text);
    return NULL;
  }

  /* response shape: {"content": [{"type": "text", "text": "..."}], ...} */
  json_t* content = json_object_get(resp, "content");
  if (!content || !json_is_array(content) || json_array_size(content) == 0) {
    /* check for API error */
    json_t* error = json_object_get(resp, "error");
    if (error) {
      json_t* emsg = json_object_get(error, "message");
      fprintf(stderr, "cl: API error: %s\n",
              emsg ? json_string_value(emsg) : "unknown");
    }
    json_decref(resp);
    return NULL;
  }

  json_t* first = json_array_get(content, 0);
  json_t* text = json_object_get(first, "text");
  const char* text_str = json_is_string(text) ? json_string_value(text) : NULL;
  char* result = text_str ? strdup(text_str) : NULL;
  json_decref(resp);
  return result;
}

/* ── Claude fetch ────────────────────────────────────────────────────────────
 */

ClResponse* fetch_response(const Config* config, const char* context,
                           const char* input, int freetext) {
  size_t prompt_cap = strlen(context) + strlen(input) + 64;
  char* prompt = malloc(prompt_cap);
  snprintf(prompt, prompt_cap, "%sUser input: \"%s\"",
           context[0] ? context : "", input);

  const char* sysprompt =
      freetext ? CL_SYSTEM_PROMPT_FREETEXT : CL_SYSTEM_PROMPT_STRICT;
  char* raw =
      claude_complete(config->api_key, config->model, sysprompt, prompt);
  free(prompt);
  if (!raw) {
    fprintf(stderr, "cl: API call failed\n");
    return NULL;
  }
  ClResponse* r = cl_response_parse(raw);
  free(raw);
  return r;
}
