#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pish.h"

/*
 * Batch mode flag. If set to 0, the shell reads from stdin. If set to 1,
 * the shell reads from a file from argv[1].
 */
static int script_mode = 0;

/*
 * Prints a prompt IF NOT in batch mode (see script_mode global flag),
 */
int prompt(void)
{
    static const char prompt[] = { 0xe2, 0x96, 0xb6, ' ', ' ', '\0' };
    if (!script_mode) {
        printf("%s", prompt);
        fflush(stdout);
    }
    return 1;
}

/*
 * Print usage error for built-in commands.
 */
void usage_error(void)
{
    fprintf(stderr, "pish: Usage error\n");
    fflush(stdout);
}

/*
 * Break down a line of input by whitespace, and put the results into
 * a struct pish_arg to be used by other functions.
 *
 * @param command   A char buffer containing the input command
 * @param arg       Broken down args will be stored here.
 */
void parse_command(char *command, struct pish_arg *arg)
{
    // 1. Clear out the arg struct
    // --- Initialize arg struc
    arg->argc = 0;
    for (int i = 0; i < MAX_ARGC; i++) {
        arg->argv[i] = NULL;
    }

    // 2. Parse the `command` buffer and update arg->argc & arg->argv.
    // --- Parse command line through tokenization
    char *tok = strtok(command, " \t\n");

    // --- While there are still tokenized strings and have not reached end of
    // command line
    while (tok && arg->argc < MAX_ARGC - 1) {
        // Update arg->argv with command line arguments
        arg->argv[arg->argc] = tok;
        // Tokenization
        tok = strtok(NULL, " \t\n");
        // Increment arg->argc
        arg->argc++;
    }
}

/*
 * Run a command.
 *
 * Built-in commands are handled internally by the pish program.
 * Otherwise, use fork/exec to create child process to run the program.
 *
 * If the command is empty, do nothing.
 * If NOT in batch mode, add the command to history file.
 */
void run(struct pish_arg *arg)
{
    // Create variable that will hold pointer to command
    char *cmd = arg->argv[0];

    // Check validity of command
    if (arg->argc == 0 || cmd == NULL) {
        return;
        ;
    }

    // Built-in command: exit
    if (strcmp(cmd, "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
    // Built-in command: cd
    else if (strcmp(cmd, "cd") == 0) {
        if (arg->argc != 2) {
            usage_error();
            return;
        }

        if (chdir(arg->argv[1]) == -1) {
            perror("cd");
        }
    }
    // Built-in command: history
    else if (strcmp(cmd, "history") == 0) {
        if (arg->argc > 1) {
            usage_error();
            return;
        }

        print_history();
    } else {
        // Fork()
        pid_t pid = fork();

        // If fork() fails
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // If child process, run command
        else if (pid == 0) {
            if (execvp(arg->argv[0], arg->argv) == -1) {
                perror("pish");
                exit(EXIT_FAILURE);
            }
        }
        // If parent process
        else {
            // Wait for the child process to finish
            wait(NULL);
        }
    }
}

/*
 * The main loop. Continuously reads input from a FILE pointer
 * (can be stdin or an actual file) until `exit` or EOF.  */
int pish(FILE *fp)
{
    // assume input does not exceed buffer size
    char buf[1024];
    struct pish_arg arg;

    // Check if file pointer is valid
    if (fp == NULL) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Loop until user enter 'exit' or end of file
    while (prompt() && fgets(buf, sizeof(buf), fp) != NULL) {
        // Parse each command line
        parse_command(buf, &arg);
        // Add command to historiy
        add_history(&arg);
        // Run parsed command
        run(&arg);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *fp;

    // If shell run without argument
    if (argc == 1) {
        // Use stdin
        fp = stdin;
    }
    // If shell run with 1 argument
    else if (argc == 2) {
        // Read from a file mode
        script_mode = 1;

        // Use argument
        fp = fopen(argv[1], "r");

        // Check if fopen worked
        if (fp == NULL) {
            // Fail
            perror("open");
            return EXIT_FAILURE;
        }
    }
    // If shell run with 2+ arguments, error
    else {
        usage_error();
        return EXIT_FAILURE;
    }

    pish(fp);

    // Close fp if not in stdin
    if (fp != stdin) {
        fclose(fp);
    }
    return EXIT_SUCCESS;
}
