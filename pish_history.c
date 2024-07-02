#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pish.h"

static char pish_history_path[1024] = { '\0' };

/*
 * Set history file path to ~/.pish_history.
 */
static void set_history_path()
{
    const char *home = getpwuid(getuid())->pw_dir;
    strncpy(pish_history_path, home, 1024);
    strcat(pish_history_path, "/.pish_history");
}

void add_history(const struct pish_arg *arg)
{
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }

    FILE *fp = fopen(pish_history_path, "a");
    if (fp == NULL) {
        perror("open history file");
        exit(EXIT_FAILURE);
    }

    // String that stores the command
    char cmds[1024] = "";
    for (int i = 0; i < arg->argc; i++) {
        // Concatenate each command onto created string
        strcat(cmds, arg->argv[i]);

        // Add space if it's not the last argument
        if (i < arg->argc - 1) {
            strcat(cmds, " ");
        }
    }
    if (arg->argc != 0) {
        // Add new line
        strcat(cmds, "\n");
    }

    // Write to file
    size_t written = fwrite(cmds, sizeof(char), strlen(cmds), fp);

    // Check if write worked
    if (written < strlen(cmds)) {
        perror("write to history file");
    }

    // Close the file
    fclose(fp);
}

void print_history()
{
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }

    // Open file
    FILE *fp = fopen(pish_history_path, "r");
    // Check if open worked
    if (fp == NULL) {
        perror("open history file");
        exit(EXIT_FAILURE);
    }

    // Tokenize path by separating with space
    char line[1024] = "";
    int count = 1;

    // Print out each command with correpsonding number
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%d %s", count++, line);
    }

    fclose(fp);
}
