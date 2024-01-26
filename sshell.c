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

//Command properties
typedef struct{
        char cmd[CMDLINE_MAX];
        char *args[CMDARGU_MAX + 1]; // +1 for the NULL pointer
        char *outputFile;
}Command;

void print_complete_message(char *cmd, int status){
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, status);
}

//check for redirection signal '>'
int redirection(char* cmd){
        if(strchr(cmd,'>') != NULL){
                return 1;
        }else{
                return 0;
        }
}

//check for append signal '>>'
int append(char* cmd){
        if(strstr(cmd,">>") != NULL){
                return 1;
        }else{
                return 0;
        }
}

//check for pipe signal '|'
int pipeline(char* cmd){
        if(strchr(cmd,'|') != NULL){
                return 1;
        }else{
                return 0;
        }       
}

// Function to initialize a Command instance
int initCommand(Command *command, char *cmdLine) {
        char commandline_copy[CMDLINE_MAX];
        char *spaceDelimiter = " ";
        char *cmd;
        int arg_count = 0;
        
         // Make a copy of cmdline to avoid modifying the original
        strcpy(commandline_copy, cmdLine);
        
        cmd = strtok(commandline_copy, spaceDelimiter);
        if (cmd != NULL) {
                strcpy(command->cmd, cmd);
                }
        
        while (cmd != NULL) {
                // check if too many argument provided
                if (arg_count >= CMDARGU_MAX) { 
                        fprintf(stderr, "Error: too many process arguments\n");
                        return 1;
                }

                command->args[arg_count] = strdup(cmd);

                //check memory allocation errors
                if (command->args[arg_count] == NULL) {
                        perror("Error: strdup failed");
                        return 1;
                }
                cmd = strtok(NULL, spaceDelimiter);
                arg_count++;
        }
        // Set the last element of command->args to NULL
        command->args[arg_count] = NULL;
        return 0;
}

// Function to initialize a Command with redirection.
int initCommand_redirection(Command *command, const char *cmdLine){
        char commandline_copy[CMDLINE_MAX];
        char *cmd;
        char *cmd_before_redir;
        char *cmd_after_redir;
        char *file;
        char *spaceDelimiter = " ";
        char *redirectionDelimiter = ">";
        int arg_count = 0;

        // Make a copy of cmdline to avoid modifying the original
        strcpy(commandline_copy, cmdLine);
        //check if the first character is redirection character and print error message if Yes
        if (commandline_copy[0] ==  '>') {
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}

        strcpy(commandline_copy, cmdLine);
        
        //separate commandline into two parts.
        cmd_before_redir =strtok(commandline_copy, redirectionDelimiter);
        cmd_after_redir = strtok(NULL, redirectionDelimiter);
        //ignore the lending space behind the redirection delimiter.
        cmd_after_redir = strtok(cmd_after_redir, spaceDelimiter);

        if(cmd_after_redir == NULL){
                command->outputFile = NULL; 
        }else{
                command->outputFile = strdup(cmd_after_redir);
        }

        cmd = strtok(cmd_before_redir, spaceDelimiter);
        if (cmd != NULL) {
                strcpy(command->cmd, cmd);
        }

        while (cmd != NULL) {
                // check if too many argument provided
                if (arg_count >= CMDARGU_MAX) { 
                        fprintf(stderr, "Error: too many process arguments\n");
                        return 1;
                }
                command->args[arg_count] = strdup(cmd);
                //check memory allocation errors
                if (command->args[arg_count] == NULL) {
                        perror("Error: strdup failed");
                        return 1;
                }
                cmd = strtok(NULL, spaceDelimiter);
                arg_count++;
        }
        // Set the last element of command->args to NULL
        command->args[arg_count] = NULL;
        return 0;
}

// Function to initialize a Command with pipeline.
int initCommand_pipeline(Command *command, const char *cmdLine){
        return 1;
}

// Function to initialize a Command with appends.
int initCommand_append(Command *command, const char *cmdLine){
        return 1;
}

// Function to execute a Command instance
int executeCommand(Command *command, int is_redirection, int is_append) {
        pid_t pid;
        int fd;
        int status;

        pid = fork();
        if (pid == 0) {
                // Child process
                if(is_redirection){
                        if(command->outputFile == NULL){
                                fprintf(stderr, "Error: no output file\n");
				exit(1); 
                        }
                        if(is_append){
                                fd = open(command->outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        }else{
                                fd = open(command->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        }
                        if (fd == -1) {
                                fprintf(stderr, "Error: cannot open output file\n");
				exit(1);
			}
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                }
                execvp(command->cmd, command->args);
                perror("execvp");
                exit(EXIT_FAILURE);
        } else if (pid > 0) {
                // Parent process
                wait(&status);
                if (WIFEXITED(status)) {
                        return 0;
                }
        } else {
                // Fork failed
                perror("fork");
                exit(EXIT_FAILURE);
        }
        return 1;
}

// Function to process pwd command.
int handle_pwd(char* directory){
        fprintf(stdout, "%s\n", directory);
        return 0;
}

// Function to process cd command.
int handle_cd(Command *command){
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

// Function to process sls command.
int handle_sls(){
        DIR *dirp;
        struct dirent *dp;
        struct stat file;
        
        dirp = opendir(".");
        if (dirp == NULL) {
                printf("Error: cannot open directory\n");
                return 1;
        }
        
        while ((dp = readdir(dirp)) != NULL) {
                //Skip "." and ".." entries
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                        continue;
                }
                // Obtain file information
                if (stat(dp->d_name, &file) == -1) {
                        perror("stat");
                        closedir(dirp);
                        return 1;
                }
                //Print entry and its size
                printf("%s (%ld bytes)\n", dp->d_name, (long)file.st_size);
        }
        closedir(dirp);
        return 0;

}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char directory[CMDLINE_MAX];
        char *nl;
        int status;
        int is_redirection;
        int is_pipe;
        int is_append;

        while (1) {
                //Initialize flag and structure.
                is_redirection = 0;
                is_pipe = 0;
                is_append = 0;
                Command *myCommand = malloc(sizeof(Command));
                char *cmdCopy;
                
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
                
                //Parsing command line
                cmdCopy = strdup(cmd);
                if(redirection(cmdCopy)){
                        is_redirection = 1;
                }

                cmdCopy = strdup(cmd);
                if(pipeline(cmdCopy)){
                        is_pipe = 1;
                }

                cmdCopy = strdup(cmd);
                if(append(cmdCopy)){
                        is_append = 1;
                }

                // Initialize Command instance
                if(is_redirection && !is_pipe){
                        status = initCommand_redirection(myCommand, cmd);
                        if(status)
                                continue;
                }else if(is_pipe){
                        status = initCommand_pipeline(myCommand, cmd);
                        if(status)
                                continue;
                }else if(!is_redirection && !is_pipe){
                        status = initCommand(myCommand, cmd);
                        if(status)
                                continue;
                }

                /* Handle built-in command "exit" */
                if (!strcmp(myCommand->args[0], "exit")) {
                        status = handle_exit();
                        print_complete_message(cmd, status);
                        break;
                }else if(!strcmp(myCommand->args[0], "cd")){ // Handle built-in command "cd" 
                        status = handle_cd(myCommand);
                        print_complete_message(cmd, status);
                }else if(!strcmp(myCommand->args[0], "pwd")){// Handle built-in command "pwd" 
                        getcwd(directory, sizeof(directory));
                        status = handle_pwd(directory);
                        print_complete_message(cmd, status);
                }else if(!strcmp(myCommand->args[0], "sls")){
                        status = handle_sls();
                        print_complete_message(cmd, status);
                }else{
                        status = executeCommand(myCommand, is_redirection, is_append);// Execute the Command                              
                        if (status){
                                continue;
                        }
                        print_complete_message(cmd, status);        
                }

                free(cmdCopy);
                free(myCommand);
        }

        return EXIT_SUCCESS;
}
