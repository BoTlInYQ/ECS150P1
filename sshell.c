#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define CMDLINE_MAX 512 //The maximum length of a command line never exceeds 512 characters.
#define CMDARGU_MAX 16 //A program has a maximum of 16 non-null arguments.
#define PATHLEN_MAX 32 //The maximum length of individual tokens never exceeds 32 characters.

//Get user's cmd
// void get_cmd(char word){
        
// }

int main(void)
{
        char cmd[CMDLINE_MAX];
        //char cwd[PATHLEN_MAX];

        pid_t pid;

        while (1) {
                char *nl;
                char *args[CMDARGU_MAX + 1]; // +1 stand for the NULL pointer
                char *token;
                //int retval;
                int status;
                int arg_counts = 0;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Handle built-in command "exit" */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                pid = fork();
                if(pid == 0){
                        token = strtok(cmd, " ");
                        while (token != NULL && arg_counts < CMDARGU_MAX) {
                                args[arg_counts++] = token;
                                token = strtok(NULL, " ");
                        }
                        args[arg_counts] = NULL;

                        execvp(args[0], args);
                        perror("execvp");
                        exit(1);
                }else if (pid > 0){
                        wait(&status);
                        if(WIFEXITED(status)){
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
                        }
                }else{
                        perror("fork");
                        exit(1);
                }

                // /* Regular command */
                // retval = system(cmd);
                // fprintf(stdout, "Return status value for '%s': %d\n",
                //         cmd, retval);
        }

        return EXIT_SUCCESS;
}
