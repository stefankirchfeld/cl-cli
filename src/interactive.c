#include "interactive.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "claude.h"
#include "history.h"

#define CL_MAX_INPUT 512
#define NUM_COMPLETIONS 3
#define DEBOUNCE_MS 1000
#define MAX_COMPLETION_LEN 1024
#define ESC_KEY 0x1b
/* ── terminal helpers ────────────────────────────────────────────────────────
 */

static struct termios orig_termios;

static void term_restore(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static void term_raw(void) {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(term_restore);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void clear_lines(int lines) {
  for (int i = 0; i < lines; i++) fputs("\033[A\033[2K", stderr);
}

#define CL_ORANGE "\033[38;5;208m"
#define CL_RESET "\033[m"

static void draw_free_text(const char* input, const char* text,
                           const char* model) {
  fprintf(stderr, "\n " CL_ORANGE "%s" CL_RESET " > %s\n\n%s\n\n", model, input,
          text);
}

static void draw(const char* input,
                 char completions[NUM_COMPLETIONS][MAX_COMPLETION_LEN],
                 int selected, int ncompletions, const char* model) {
  fprintf(stderr, "\033[2K\r " CL_ORANGE "%s" CL_RESET " > %s\n", model, input);
  for (int i = 0; i < ncompletions; i++) {
    if (i == selected)
      fprintf(stderr, "\033[2K\r  \033[7m %s \033[m\n", completions[i]);
    else
      fprintf(stderr, "\033[2K\r    %s\n", completions[i]);
  }
}

/* ── main interactive loop ───────────────────────────────────────────────────
 */

int interactive_mode(const Config* config, const char* context, int freetext,
                     const char* initial) {
  char input[CL_MAX_INPUT] = "";
  int input_len = 0;
  char completions[NUM_COMPLETIONS][MAX_COMPLETION_LEN] = {{0}};
  int selected = 0;
  int ncompletions = 0;
  int drawn_lines = 1;

  /* pre-populate from argv */
  if (initial && initial[0]) {
    snprintf(input, sizeof(input), "%s", initial);
    input_len = (int)strlen(input);
  }

  History* hist = history_load();

  term_raw();
  fprintf(stderr, "\n " CL_ORANGE "%s" CL_RESET " > %s\n", config->model,
          input);

  /* if pre-populated, trigger fetch immediately */
  struct timeval last_key = {0};
  int pending_fetch = (input_len > 0);

  while (1) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval timeout;
    if (pending_fetch) {
      struct timeval now;
      gettimeofday(&now, NULL);
      long elapsed_ms = (now.tv_sec - last_key.tv_sec) * 1000 +
                        (now.tv_usec - last_key.tv_usec) / 1000;
      long remaining = DEBOUNCE_MS - elapsed_ms;
      if (remaining <= 0) {
        pending_fetch = 0;
        if (input_len > 0) {
          history_append(hist, input);
          hist->pos = -1;
          ClResponse* resp = fetch_response(config, context, input, freetext);
          if (resp) {
            if (resp->type == CL_RESP_FREETEXT) {
              clear_lines(drawn_lines);
              draw_free_text(input, resp->text, config->model);
              input[0] = '\0';
              input_len = 0;
              ncompletions = 0;
              selected = 0;
              /* draw_free_text left cursor after its output; draw fresh prompt
               */
              drawn_lines = 1;
              draw(input, completions, selected, ncompletions, config->model);
              cl_response_free(resp);
              continue;
            } else if (resp->type == CL_RESP_COMPLETIONS && resp->nlines > 0) {
              ncompletions = resp->nlines;
              for (int i = 0; i < resp->nlines && i < NUM_COMPLETIONS; i++)
                snprintf(completions[i], MAX_COMPLETION_LEN, "%s",
                         resp->lines[i]);
              selected = 0;
            }
            cl_response_free(resp);
          }
        }
        clear_lines(drawn_lines);
        drawn_lines = 1 + ncompletions;
        draw(input, completions, selected, ncompletions, config->model);
        continue;
      }
      timeout.tv_sec = remaining / 1000;
      timeout.tv_usec = (remaining % 1000) * 1000;
    } else {
      timeout.tv_sec = 10;
      timeout.tv_usec = 0;
    }

    int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);
    if (ready <= 0) continue;

    unsigned char ch;
    int n = read(STDIN_FILENO, &ch, 1);
    if (n == 0) {
      /* EOF (Ctrl+D) — treat as cancel */
      clear_lines(drawn_lines);
      history_free(hist);
      return 1;
    }
    if (n != 1) continue;

    /* escape sequences (arrow keys) */
    if (ch == ESC_KEY) {
      unsigned char seq[2];
      struct timeval t = {0, 50000};
      fd_set f;
      FD_ZERO(&f);
      FD_SET(STDIN_FILENO, &f);
      if (select(STDIN_FILENO + 1, &f, NULL, NULL, &t) > 0 &&
          read(STDIN_FILENO, seq, 1) == 1 && seq[0] == '[') {
        FD_ZERO(&f);
        FD_SET(STDIN_FILENO, &f);
        if (select(STDIN_FILENO + 1, &f, NULL, NULL, &t) > 0 &&
            read(STDIN_FILENO, seq + 1, 1) == 1) {
          if (seq[1] == 'A') { /* up */
            if (ncompletions > 0) {
              selected = (selected - 1 + ncompletions) % ncompletions;
            } else if (hist->count > 0) {
              if (hist->pos < 0) hist->pos = hist->count;
              if (hist->pos > 0) {
                hist->pos--;
                snprintf(input, sizeof(input), "%s", hist->entries[hist->pos]);
                input_len = (int)strlen(input);
                pending_fetch = 0;
              }
            }
          } else if (seq[1] == 'B') { /* down */
            if (ncompletions > 0) {
              selected = (selected + 1) % ncompletions;
            } else if (hist->pos >= 0) {
              hist->pos++;
              if (hist->pos >= hist->count) {
                hist->pos = -1;
                input[0] = '\0';
                input_len = 0;
              } else {
                snprintf(input, sizeof(input), "%s", hist->entries[hist->pos]);
                input_len = (int)strlen(input);
              }
              pending_fetch = 0;
            }
          }
          clear_lines(drawn_lines);
          drawn_lines = 1 + ncompletions;
          draw(input, completions, selected, ncompletions, config->model);
        }
      }
      continue;
    }

    /* ctrl-c: cancel */
    if (ch == 3) {
      clear_lines(drawn_lines);
      history_free(hist);
      return 1;
    }

    /* enter: confirm selection */
    if (ch == '\r' || ch == '\n') {
      if (ncompletions > 0) {
        clear_lines(drawn_lines);
        fputs(completions[selected], stdout);
        fflush(stdout);
        history_free(hist);
        return 0;
      } else if (input_len) {
        pending_fetch = 1;
      }
      continue;
    }

    /* backspace */
    if (ch == 127 || ch == '\b') {
      if (input_len > 0) {
        input[--input_len] = '\0';
        ncompletions = 0;
        selected = 0;
      }
    } else if (ch >= 32 && input_len < CL_MAX_INPUT - 1) {
      input[input_len++] = (char)ch;
      input[input_len] = '\0';
      ncompletions = 0;
      selected = 0;
    }

    gettimeofday(&last_key, NULL);
    pending_fetch = 1;

    clear_lines(drawn_lines);
    drawn_lines = 1 + ncompletions;
    draw(input, completions, selected, ncompletions, config->model);
  }
}
