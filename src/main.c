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
    if (!config) return 1;

    if (!config->api_key[0]) {
        fprintf(stderr, "cl: api_key not set in ~/.config/cl/config.ini\n");
        free(config);
        return 1;
    }

    /* parse -f flag */
    int freetext = 0;
    int first_arg = 1;
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
