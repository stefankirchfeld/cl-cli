#pragma once

/* Call the Anthropic Messages API. Returns heap-allocated response text
   or NULL on error. Caller frees. */
char *claude_complete(const char *api_key, const char *model,
                      const char *system_prompt, const char *user_prompt);
