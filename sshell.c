#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512 //The maximum length of a command line never exceeds 512 characters.
#define CMDARGU_MAX 16 //A program has a maximum of 16 non-null arguments.
#define TOKEN_MAX 32 //The maximum length of individual tokens never exceeds 32 characters.

/*Command properties*/
struct Command{
        char cmd[CMDLINE_MAX];
        char *args[CMDARGU_MAX + 1]; // +1 for the NULL pointer
        char *outputFile;
};

void print_complete_message(char *cmd, int status){
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, status);
}

/*check for redirection signal '>'*/
int redirection(char* cmd){
        if(strchr(cmd,'>') != NULL && strstr(cmd,">>") == NULL){
                return 1;
        }else{
                return 0;
        }
}

/*check for append signal '>>'*/
int append(char* cmd){
        if(strstr(cmd,">>") != NULL){
                return 1;
        }else{
                return 0;
        }
}

/*check for pipe signal '|'*/
int pipeline(char* cmd){
        if(strchr(cmd,'|') != NULL){
                return 1;
        }else{
                return 0;
        }       
}

// Function to initialize a Command instance
int initCommand(struct Command *command, const char *cmdLine) {
        char *token;
        char *spaceDelimiter = " ";
        int arg_count = 0;

        strncpy(command->cmd, cmdLine, sizeof(command->cmd));
        command->cmd[sizeof(command->cmd) - 1] = '\0';

        // Initialize args array
        token = strtok(command->cmd, spaceDelimiter);
        while (token != NULL && arg_count < CMDARGU_MAX) {
                if (strlen(token) < TOKEN_MAX){
                        command->args[arg_count++] = token;
                        token = strtok(NULL, spaceDelimiter);
                }else{
                        fprintf(stderr, "Error: reach token maximum\n");
                        return 1;
                }
                if(arg_count >= CMDARGU_MAX){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return 1;
                }
        }

        // Set remain args to NULL
        while(arg_count < CMDARGU_MAX){
                command->args[arg_count] = NULL;
                arg_count++; 
        }
        return 0;
}

// Function to initialize a Command with redirection.
int initCommand_redirection(struct Command *command, const char *cmdLine){
        char *token;
        char *spaceDelimiter = " ";
        int arg_count = 0;

        strncpy(command->cmd, cmdLine, sizeof(command->cmd));
        command->cmd[sizeof(command->cmd) - 1] = '\0';

        // Initialize args array
        token = strtok(command->cmd, spaceDelimiter);

        /*check if the first character is redirection character and print error message if Yes*/
        if (token[0] ==  '>') {
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}


        while (token != NULL && arg_count < CMDARGU_MAX) {
                if (strlen(token) < TOKEN_MAX){
                        command->args[arg_count++] = token;
                        token = strtok(NULL, spaceDelimiter);
                }else{
                        fprintf(stderr, "Error: reach token maximum\n");
                        return 1;
                }
                if(arg_count >= CMDARGU_MAX){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return 1;
                }
        }

        // Set remain args to NULL
        while(arg_count < CMDARGU_MAX){
                command->args[arg_count] = NULL;
                arg_count++; 
        }
        return 0;
}

// Function to initialize a Command with pipeline.
int initCommand_pipeline(struct Command *command, const char *cmdLine){

}

// Function to initialize a Command with appends.
int initCommand_append(struct Command *command, const char *cmdLine){

}

// Function to execute a Command instance
void executeCommand(struct Command *command) {
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
                        print_complete_message(command->cmd, status);
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
                chdir(command->args[1]);
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

int handle_sls(){
        DIR *dirp;
        struct dirent *dp;
        struct stat file_stat;
        
        dirp = opendir(".");
        if (dirp == NULL) {
                printf("Error: cannot open directory\n");
                return 1;
        }
        
        while ((dp = readdir(dirp)) != NULL) {
                // Skip "." and ".." entries
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                        continue;
                }
                // Obtain file information
                if (stat(dp->d_name, &file_stat) == -1) {
                        perror("stat");
                        closedir(dirp);
                        return 1;
                }
                // Print entry and its size
                printf("%s (%ld bytes)\n", dp->d_name, (long)file_stat.st_size);
        }
        closedir(dirp);
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
                if(redirection(cmd)){
                        status = initCommand_redirection(&myCommand, cmd);
                        if(status)
                                continue;
                }else if(pipeline(cmd)){
                        status = initCommand_pipeline(&myCommand, cmd);
                        if(status)
                                continue;
                }else if(append(cmd)){
                        status = initCommand_append(&myCommand, cmd);
                        if(status)
                                continue;
                }else{
                        status = initCommand(&myCommand, cmd);
                        if(status)
                                continue;
                }
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
                }else if(!strcmp(myCommand.args[0], "sls")){
                        status = handle_sls();
                        print_complete_message(cmd, status);
                }else{
                        executeCommand(&myCommand);// Execute the Command
                }
        }

        return EXIT_SUCCESS;
}
