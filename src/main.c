#include "claude.h"
#include "config.h"
#include "context.h"
#include "interactive.h"
#include "pipe.h"

#include <llense/libcurl/curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  Config *config = config_load();
  if (!config)
    return 1;

  if (!config->api_key[0]) {
    fprintf(stderr, "cl: api_key not set in ~/.config/cl/config.ini\n");
    free(config);
    return 1;
  }

  /* parse flags */
  int freetext = 0;
  int first_arg = 1;

  if (argc > 1 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    puts(
        "Usage: cl [-f] [prompt...]\n"
        "\n"
        "Modes:\n"
        "  cl                     Interactive TUI — type a query, get 3 shell "
        "completions\n"
        "  cl <prompt>            Interactive TUI pre-populated with <prompt>\n"
        "  <cmd> | cl <question>  Pipe mode — send stdin + question to Claude, "
        "print answer\n"
        "\n"
        "Note: completions are printed to stdout. Only the Ctrl+K / Ctrl+F "
        "bindings\n"
        "inject the result directly into your shell readline buffer.\n"
        "When using 'cl' as a plain command, copy/paste the output manually.\n"
        "\n"
        "Flags:\n"
        "  -f          Allow free-text responses (default: completions only)\n"
        "  -h, --help  Show this help\n"
        "\n"
        "Key bindings (add to ~/.bashrc via shell/cl.bash):\n"
        "  Ctrl+K      Open interactive TUI (picks up current readline line as "
        "prompt)\n"
        "  Ctrl+F      Same, but allows free-text responses\n"
        "\n"
        "Interactive TUI keys:\n"
        "  Type        Enter your query (debounce triggers Claude after 1s "
        "pause)\n"
        "  Up/Down     Browse history (no completions) or select completion\n"
        "  Enter       Confirm selected completion — lands in your shell "
        "readline buffer\n"
        "  Ctrl+C      Cancel\n"
        "\n"
        "Examples:\n"
        "  cl find large files modified today\n"
        "  cl grep for TODO but skip build dir\n"
        "  git log --oneline -10 | cl -f \"what changed recently?\"\n"
        "  make 2>&1 | cl -f \"what is the error and how to fix it\"\n"
        "  ps aux | cl \"kill command for the ssh-agent process\"\n"
        "\n"
        "Config: ~/.config/cl/config.ini\n"
        "History: ~/.config/cl/history  (last 100 queries)");
    return 0;
  }

  if (argc > 1 && strcmp(argv[1], "-f") == 0) {
    freetext = 1;
    first_arg = 2;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  char *context = context_collect(config);

  int rc;
  if (!isatty(STDIN_FILENO)) {
    /* pipe mode: join remaining argv as the question */
    char question[1024] = "";
    for (int i = first_arg; i < argc; i++) {
      if (i > first_arg)
        strncat(question, " ", sizeof(question) - strlen(question) - 1);
      strncat(question, argv[i], sizeof(question) - strlen(question) - 1);
    }
    rc = pipe_mode(config, context, question, freetext);
  } else {
    /* interactive mode: join remaining argv as pre-populated input */
    char initial[1024] = "";
    for (int i = first_arg; i < argc; i++) {
      if (i > first_arg)
        strncat(initial, " ", sizeof(initial) - strlen(initial) - 1);
      strncat(initial, argv[i], sizeof(initial) - strlen(initial) - 1);
    }
    rc = interactive_mode(config, context, freetext,
                          initial[0] ? initial : NULL);
  }

  free(context);
  free(config);
  curl_global_cleanup();
  return rc;
}
