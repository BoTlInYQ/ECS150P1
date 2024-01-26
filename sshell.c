#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

#define CMDLINE_MAX 512 //The maximum length of a command line never exceeds 512 characters.
#define CMDARGU_MAX 16 //A program has a maximum of 16 non-null arguments.
#define TOKEN_MAX 32 //The maximum length of individual tokens never exceeds 32 characters.

/*Command properties*/
struct Command{
        char cmd[CMDLINE_MAX];
        char *args[CMDARGU_MAX + 1]; // +1 for the NULL pointer
};

void print_complete_message(char *cmd, int status){
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
}

// Function to initialize a Command instance
void initCommand(struct Command *command, const char *cmdLine) {
        char *token;
        int argCount = 0;

        strncpy(command->cmd, cmdLine, sizeof(command->cmd));
        command->cmd[sizeof(command->cmd) - 1] = '\0';

        // Initialize args array
        token = strtok(command->cmd, " ");
        while (token != NULL && argCount < CMDARGU_MAX) {
                if (strlen(token) < TOKEN_MAX){
                        command->args[argCount++] = token;
                        token = strtok(NULL, " ");
                }else{
                        fprintf(stderr, "Error: reach token maximum\n");
                }
        }
        command->args[argCount] = NULL; // Set the last element to NULL
}

// Function to execute a Command instance
void executeCommand(const struct Command *command) {
        pid_t pid;
        int status;

        pid = fork();
        if (pid == 0) {
                // Child process
                execvp(command->args[0], command->args);
                perror("execvp");
                exit(EXIT_FAILURE);
        } else if (pid > 0) {
                // Parent process
                wait(&status);
                if (WIFEXITED(status)) {
                        fprintf(stderr, "+ completed '%s' [%d]\n", command->cmd, WEXITSTATUS(status));
                }
        } else {
                // Fork failed
                perror("fork");
                exit(EXIT_FAILURE);
        }
}

// Function to process pwd command.
int handle_pwd(){
        char cwd[CMDLINE_MAX];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
                return 0;
        } else {
                perror("getcwd");
                return 1;
        }
}

// Function to process cd command.
int handle_cd(const struct Command *command){
        DIR * dir;

        if (command->args[1] != NULL) {
                dir = opendir(command->args[1]);
                if(dir == NULL){
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        return 1;         
                }
                chdir(command ->args[1]);
                return 0;
        } else {
                fprintf(stderr, "cd: missing argument\n");
                return 1;
        }  
}

// Function to process exit command.
int handle_exit(){
        fprintf(stderr, "Bye...\n");
        return 0;
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char *nl;
        int status;
        struct Command myCommand;

        while (1) {
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

                // Initialize Command instance
                initCommand(&myCommand, cmd);

                /* Handle built-in command "exit" */
                if (!strcmp(myCommand.args[0], "exit")) {
                        status = handle_exit();
                        print_complete_message(cmd, status);
                        break;
                }else if(!strcmp(myCommand.args[0], "cd")){ /* Handle built-in command "cd" */
                        status = handle_cd(&myCommand);
                        print_complete_message(cmd, status);
                }else if(!strcmp(myCommand.args[0], "pwd")){/* Handle built-in command "pwd" */
                        status = handle_pwd();
                        print_complete_message(cmd, status);
                }else{
                        executeCommand(&myCommand);// Execute the Command
                }
        }

        return EXIT_SUCCESS;
}
