#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDARGU_MAX 16 //A program has a maximum of 16 non-null arguments.
#define CMDLINE_MAX 512 //The maximum length of a command line never exceeds 512 characters.
#define TOKEN_MAX 32 //The maximum length of individual tokens never exceeds 32 characters.

//Command properties
typedef struct{
        char cmd[CMDLINE_MAX];
        char *args[CMDARGU_MAX + 1]; // +1 for the NULL pointer
        char *outputFile;
}Command;

typedef struct{
        Command *pipe_commands1;
        Command *pipe_commands2;
        Command *pipe_commands3;
        Command *pipe_commands4;
}Pipe;

//Global Variable
int pipe_index = 1;

//Function to print completation message single command
void print_complete_message(char *cmd, int status){
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, status);
}

//Function to print completation message for pipeline command
void print_pipeline_complete_message(char *cmd, int *status, int pipe_index){
        if(pipe_index == 1){
                fprintf(stderr, "+ completed '%s' [%d][%d]\n", cmd, status[0],status[1]);
        }
        if(pipe_index == 2){
                fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n", cmd, status[0],status[1],status[2]);
        }
        if(pipe_index == 3){
                fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n", cmd, status[0],status[1],status[2],status[3]);
        }
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
int init_command(Command *command, char *cmdLine) {
        char *cmd;
        char *commandline_copy = malloc(CMDLINE_MAX * sizeof(char));
        char *space_delimiter = " ";
        int arg_count = 0;
        
         // Make a copy of cmdline to avoid modifying the original
        strcpy(commandline_copy, cmdLine);
        
        cmd = strtok(commandline_copy, space_delimiter);
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
                cmd = strtok(NULL, space_delimiter);
                arg_count++;
        }
        while(arg_count <= CMDARGU_MAX){
                // Set unused element of command->args to NULL
                command->args[arg_count] = NULL;
                arg_count++;
        }
        free(commandline_copy);
        return 0;
}

// Function to initialize a Command with redirection.
int init_command_redirection(Command *command, char *cmdLine){
        char *cmd;
        char *cmd_after_redir;
        char *cmd_before_redir;
        char *commandline_copy = malloc(CMDLINE_MAX * sizeof(char));
        char *redirection_meta = ">";
        char *space = " ";
        int arg_count = 0;

        // Make a copy of cmdline to avoid modifying the original
        strcpy(commandline_copy, cmdLine);
        //check if the first character is redirection character and print error message if Yes
        if (strncmp(commandline_copy, redirection_meta, 1) == 0) {
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}

        strcpy(commandline_copy, cmdLine);
        
        //separate commandline into two parts.
        cmd_before_redir =strtok(commandline_copy, redirection_meta);
        cmd_after_redir = strtok(NULL, redirection_meta);
        //ignore the lending space behind the redirection delimiter.
        cmd_after_redir = strtok(cmd_after_redir, space);

        if(cmd_after_redir == NULL){
                command->outputFile = NULL; 
        }else{
                command->outputFile = strdup(cmd_after_redir);
        }

        cmd = strtok(cmd_before_redir, space);
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
                cmd = strtok(NULL, space);
                arg_count++;
        }
        while(arg_count <= CMDARGU_MAX){
                // Set unused element of command->args to NULL
                command->args[arg_count] = NULL;
                arg_count++;
        }

        free(commandline_copy);
        return 0;
}

// Function to initialize a Command with pipeline.
int init_command_pipeline(Pipe *pipe, char *cmdLine){
        char *commandline_copy = malloc(CMDLINE_MAX * sizeof(char));
        char *pipe_meta = "|";
        char *fragment1;
        char *fragment2;
        char *fragment3;
        char *fragment4;
        char *fragment_left;
        char *fragment_right;

        // Make a copy of cmdline to avoid modifying the original
        strcpy(commandline_copy, cmdLine);
        //check if the first character is redirection character and print error message if Yes
        if (strncmp(commandline_copy, pipe_meta, 1) == 0) {
                //empty 1st pipe
		fprintf(stderr, "Error: missing command\n");
		return 1;
	}
        //separate command
        strcpy(commandline_copy, cmdLine);
        fragment_left = strtok(commandline_copy, pipe_meta);
        fragment_right = strtok(NULL, pipe_meta);

        if(fragment_left != NULL){
                //1st pipe
                strcpy(fragment1,fragment_left);
                if(redirection(fragment1)){
                        //mislocated: have redirection in first pipe.
                        fprintf(stderr, "Error: mislocated output rediretion\n");
                        return 1;
                }
                if(!strcmp(fragment1, " ")){
                        //emptyspace 1st pipe
                        fprintf(stderr, "Error: missing command\n");
                        return 1;
                }
        }

        //check if there is a second pipe signal
        if(pipeline(fragment_right)){
                pipe_index++; //there is a second pipe signal

                //separate command
                strcpy(commandline_copy,fragment_right);
                fragment_left = strtok(commandline_copy,pipe_meta);
                fragment_right = strtok(NULL, pipe_meta);

                //check if second pipe have command.
                if(fragment_left != NULL){
                        strcpy(fragment2,fragment_left);
                        if(redirection(fragment1) || redirection(fragment2)){
                                //mislocated: have redirection in first or second pipe when third pipe exist.
                                fprintf(stderr, "Error: mislocated output rediretion\n");
                                return 1;
                        }
                        if(!strcmp(fragment2, " ")){
                                //emptyspace 2st pipe
                                fprintf(stderr, "Error: missing command\n");
                                return 1;
                        }

                        //check if there is a third pipe signal
                        if(pipeline(fragment_right)){
                                pipe_index++; // there is a third pipe signal
                                
                                //separate command
                                strcpy(commandline_copy,fragment_right);
                                fragment_left = strtok(commandline_copy,pipe_meta);
                                fragment_right = strtok(NULL, pipe_meta);
                                strcpy(fragment3,fragment_left);
                                strcpy(fragment4,fragment_right);
                                if(redirection(fragment1) || redirection(fragment2) || redirection(fragment3)){
                                        //mislocated: have redirection in third pipe when fourth pipe exist.
                                        fprintf(stderr, "Error: mislocated output rediretion\n");
                                        return 1;
                                }
                                if(!strcmp(fragment3," ") || !strcmp(fragment4," ")){
                                        //emptyspace 3rd/4th pipe
                                        fprintf(stderr, "Error: missing command\n");
                                        return 1;
                                }
                        }else{
                                strcpy(fragment3,fragment_right);
                                if(!strcmp(fragment3, " ")){
                                        //emptyspace 3rd pipe
                                        fprintf(stderr, "Error: missing command\n");
                                        return 1;
                                }
                        }

                }else{
                        //missing 2nd pipe
                        fprintf(stderr, "Error: missing command\n");
                        return 1;
                }
        }else{
                strcpy(fragment2,fragment_right);
                if(!strcmp(fragment2, " ")){
                        //emptyspace 3rd pipe
                        fprintf(stderr, "Error: missing command\n");
                        return 1;
                }

        }
        init_command(pipe->pipe_commands1,fragment1);

        if(pipe_index == 1){
                init_command_redirection(pipe->pipe_commands2,fragment2);
        }
        if(pipe_index == 2){
                init_command(pipe->pipe_commands2,fragment2);
                init_command_redirection(pipe->pipe_commands3,fragment3);
        }
        if(pipe_index == 3){
                init_command(pipe->pipe_commands2,fragment2);
                init_command(pipe->pipe_commands3,fragment3);
                init_command_redirection(pipe->pipe_commands4,fragment4);
        }

        // Free allocated memory
        free(commandline_copy);

        return 0;

}



// Function to execute a Command and handle with the redirection and append function without pipeline
int execute_command(Command *command, int is_redirection, int is_append) {
        int fd;
        int executeStatus = 1;
        int status;
        pid_t pid;

        pid = fork();
        if (pid == 0) {
                // Child process
                if(is_redirection){
                        if(command->outputFile == NULL){
                                fprintf(stderr, "Error: no output file\n");
				return executeStatus; 
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
                fprintf(stderr, "Error: command not found\n");
                print_complete_message(command->args[0], executeStatus);
                exit(1);
        } else if (pid > 0) {
                // Parent process
		waitpid(pid, &status, 0);
		executeStatus = WEXITSTATUS(status);
        } else {
                // Fork failed
                perror("fork");
                exit(1);
        }
        return executeStatus;
}

int* execute_command_pipe(Pipe *pipeline, int is_redirection){

        int fd1[2];
	int fd2[2];
	int fd3[2];

	pid_t pid1;
	pid_t pid2;
	pid_t pid3;
	pid_t pid4;

        static int executeStatus[4]; // return exit values
	int status[4];

        	if (pipe_index == 1) { // Exactly 1 pipe
		pipe(fd1); // Create a pipe
		pid1 = fork(); // Execute the first command in pipe
		if (pid1 == 0) {
			// Child
			close(fd1[0]); // Close the pipe input
			dup2(fd1[1], STDOUT_FILENO); // Output to pipe
			close(fd1[1]); // Close the pipe output
			execvp(pipeline->pipe_commands1->cmd, pipeline->pipe_commands1->args);
			perror("execvp");
			exit(1);

		} else if (pid1 != 0) {
			// Parent
			waitpid(pid1, &status[0],0);
			executeStatus[0] = WEXITSTATUS(status[0]);
		}
		pid2 = fork(); //Execute the second command in pipe
		if (pid2 == 0) {
			// Child
			if (is_redirection) { // If there is redirection
				close(fd1[1]); // Close the pipe output
				dup2(fd1[0], STDIN_FILENO); // Input from pipe
				close(fd1[0]); // Close the pipe input
				if (pipeline->pipe_commands2->outputFile == NULL) {
					fprintf(stderr, "Error: no output file\n");
					exit(1);
				}
				int fdirection;
				fdirection = open(pipeline->pipe_commands2->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdirection == -1) {
					fprintf(stderr, "Error: cannot open output file\n");
					exit(1);
				}
				dup2(fdirection, STDOUT_FILENO); // Output to file
				close(fdirection); // Close file
				close(fd1[1]); // Close pipe output
				dup2(fd1[0], STDIN_FILENO); // Input from pipe
				close(fd1[0]); // Close pipe input
				execvp(pipeline->pipe_commands2->cmd, pipeline->pipe_commands2->args);
				perror("execvp");
				exit(1);
			} else { // No redirection
				close(fd1[1]); // Close the pipe output
				dup2(fd1[0], STDIN_FILENO); // Input from pipe
				close(fd1[0]); // Close the pipe input
				execvp(pipeline->pipe_commands2->cmd, pipeline->pipe_commands2->args);
				perror("execvp");
				exit(1);
			}
		} else if (pid2 != 0) {
			// Parent
			// Totally close pipe
			close(fd1[0]);
			close(fd1[1]);
			waitpid(pid2, &status[1],0);
			executeStatus[1] = WEXITSTATUS(status[1]);
		}

	} else if(pipe_index == 2) { // 2 Pipes
		pipe(fd1); // First pipe
		pipe(fd2); // Second pipe
		pid1 = fork();
		if (pid1 == 0) { // Execute the first command in pipe
			// Child
			close(fd1[0]); // Close pipe1 input
			dup2(fd1[1], STDOUT_FILENO); // Output to pipe1
			close(fd1[1]); // Close pipe1 output
			execvp(pipeline->pipe_commands1->cmd, pipeline->pipe_commands1->args);
			perror("execvp");
			exit(1);
		} else if (pid1 != 0) {
			// Parent
			waitpid(pid1, &status[0],0);
			executeStatus[0] = WEXITSTATUS(status[0]);
		}
		pid2 = fork();
		if (pid2 == 0) { // Execute the second command in pipe
			close(fd1[1]); // Close pipe1 output
			dup2(fd1[0], STDIN_FILENO); // Input from pipe1
			close(fd1[0]); // Close pipe1 input
			// Pipe 1 ends
			// Pipe 2 start
			close(fd2[0]); // Close pipe2 input
			dup2(fd2[1], STDOUT_FILENO); // Output to pipe2
			close(fd2[1]);// Close pipe2 output
			execvp(pipeline->pipe_commands2->cmd, pipeline->pipe_commands2->args);
			perror("execvp");
			exit(1);
		} else if (pid2 != 0) {
			// Parent
			// Totally close pipe1
			close(fd1[0]);
			close(fd1[1]);
			waitpid(pid2, &status[1],0);
			executeStatus[1] = WEXITSTATUS(status[1]);
		}
		pid3 = fork();
		if (pid3 == 0) { // Execute the third command in pipe
			if (is_redirection) { // If there is redirection
				close(fd2[1]); // Close pipe2
				dup2(fd2[0], STDIN_FILENO); // Input from pipe2
				close(fd2[0]); // Close pipe2 input
				if (pipeline->pipe_commands3->outputFile == NULL) {
					fprintf(stderr, "Error: no output file\n");
					exit(1);
				}
				int fdirection;
				fdirection = open(pipeline->pipe_commands3->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdirection == -1) {
					fprintf(stderr, "Error: cannot open output file\n");
					exit(1);
				}
				dup2(fdirection, STDOUT_FILENO); // Output to file
				close(fdirection); // Close file
				close(fd2[1]); // Clode pipe2 output
				dup2(fd2[0], STDIN_FILENO); // Input from pipe2
				close(fd2[0]); // Close pipe2 input
				execvp(pipeline->pipe_commands3->cmd, pipeline->pipe_commands3->args);
				perror("execvp");
				exit(1);
			} else { // No redirection
				close(fd2[1]); // Close pipe2 output
				dup2(fd2[0], STDIN_FILENO); // Input from pipe2
				close(fd2[0]); // Close pipe2 input
				execvp(pipeline->pipe_commands3->cmd, pipeline->pipe_commands3->args);
				perror("execvp");
				exit(1);
			}
		}else if (pid3 != 0) {
			// Parent
			// Totally close pipe2
			close(fd2[0]);
			close(fd2[1]);
			waitpid(pid3, &status[2],0);
			executeStatus[2] = WEXITSTATUS(status[2]);
		}

	} else if (pipe_index == 3) { // 3 pipes
		pipe(fd1);
		pipe(fd2);
		pipe(fd3); // Third pipe
		pid1 = fork();
		if (pid1 == 0) { // Execute the first command in pipe
			// Child
			close(fd1[0]); // Close pipe1 input
			dup2(fd1[1], STDOUT_FILENO); // Output to pipe1
			close(fd1[1]); // Close pipe1 output
			execvp(pipeline->pipe_commands1->cmd, pipeline->pipe_commands1->args);
			perror("execvp");
			exit(1);
		} else if (pid1 != 0) {
			//Parent
			waitpid(pid1, &status[0],0);
			executeStatus[0] = WEXITSTATUS(status[0]);
		}

		pid2 = fork();
		if (pid2 == 0) { // Execute the second command in pipe
			close(fd1[1]); // Close pipe1 output
			dup2(fd1[0], STDIN_FILENO); // Input from pipe1
			close(fd1[0]); // Close pipe1 input
			// Pipe 1 ends
			// Pipe 2 starts
			close(fd2[0]); // Close pipe2 input
			dup2(fd2[1], STDOUT_FILENO); // Output to pipe2
			close(fd2[1]); // Close pipe2 output
			execvp(pipeline->pipe_commands2->cmd, pipeline->pipe_commands2->args);
			perror("execvp");
			exit(1);
		} else if (pid2 != 0) {
			// Parent
			// Totally close pipe1
			close(fd1[0]);
			close(fd1[1]);
			waitpid(pid2, &status[1],0);
			executeStatus[1] = WEXITSTATUS(status[1]);
		}

		pid3 = fork();
		if (pid3 == 0) { // Execute the third command in pipe
			close(fd2[1]); // Close pipe2 output
			dup2(fd2[0], STDIN_FILENO); // Input from pipe2
			close(fd2[0]); // Close pipe2 input
			// Pipe2 ends
			// Pipe2 starts
			close(fd3[0]); // Close pipe3 input
			dup2(fd3[1],STDOUT_FILENO); // Output to pipe3
			close(fd3[1]); // Close pipe3 output
			execvp(pipeline->pipe_commands3->cmd, pipeline->pipe_commands3->args);
			perror("execvp");
			exit(1);
		} else if (pid3 != 0) {
			// Parent
			// Totally close pipe2
			close(fd2[0]);
			close(fd2[1]);
			waitpid(pid3, &status[2],0);
			executeStatus[2] = WEXITSTATUS(status[2]);
		}
		pid4 = fork();
		if (pid4 == 0) { // Execute the fouth command in pipe
			if (is_redirection) { // If there is redirection
				close(fd3[1]); // Close pipe3 output
				dup2(fd3[0], STDIN_FILENO); // Input from pipe3
				close(fd3[0]); // Close pipe3 input

				if (pipeline->pipe_commands4->outputFile == NULL) {
					fprintf(stderr, "Error: no output file\n");
					exit(1);
				}

				int fdirection;
				fdirection = open(pipeline->pipe_commands4->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdirection == -1) {
					fprintf(stderr, "Error: cannot open output file\n");
					exit(1);
				}
				dup2(fdirection, STDOUT_FILENO); // Output to file
				close(fdirection); // Close file
				close(fd3[1]); // Close pipe3 output
				dup2(fd3[0], STDIN_FILENO); // Input from pipe3
				close(fd3[0]); // Close pipe3 input
				execvp(pipeline->pipe_commands4->cmd, pipeline->pipe_commands4->args);
				perror("execvp");
				exit(1);
			} else { // No redirection
				close(fd3[1]); // Close pipe3 output
				dup2(fd3[0], STDIN_FILENO); // Input from pipe3
				close(fd3[0]); // Close pipe3 input
				execvp(pipeline->pipe_commands4->cmd, pipeline->pipe_commands4->args);
				perror("execvp");
				exit(1);
			}
		} else if (pid4 != 0) {
			// Parent
			// Totally close pipe3
			close(fd3[0]);
			close(fd3[1]);
			waitpid(pid4, &status[3],0);
			executeStatus[3] = WEXITSTATUS(status[3]);
		}
	}
	return executeStatus;
}
// Function to process pwd command.
int handle_pwd(){
        char cwd[CMDLINE_MAX];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                fprintf(stdout, "%s\n", cwd);
                return 0;
        } else {
                perror("getcwd");
                return 1;
        }
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
        char *nl;
        int is_append;
        int is_pipe;
        int is_redirection;
        int status;
        int *pipestatus;

        while (1) {
                //Initialize flag and structure.
                char *cmdCopy;
                Command *myCommand = malloc(sizeof(Command));
                Pipe *myPipe = malloc(sizeof(Pipe));
                is_append = 0;
                is_pipe = 0;
                is_redirection = 0;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                // Get command line 
                fgets(cmd, CMDLINE_MAX, stdin);

                // Print command line if stdin is not provided by terminal 
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                // Remove trailing newline from command line 
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';
                

                //Parsing command line checking flags
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
                        status = init_command_redirection(myCommand, cmd);
                        if(status)
                                continue;
                }else if(is_pipe){
                        status = init_command_pipeline(myPipe, cmd);
                        if(status)
                                continue;
                }else if(!is_redirection && !is_pipe){
                        status = init_command(myCommand, cmd);
                        if(status)
                                continue;
                }

                if (!strcmp(myCommand->args[0], "exit")) {// Handle built-in command "exit" 
                        status = handle_exit();
                        print_complete_message(cmd, status);
                        break;
                }else if(!strcmp(myCommand->args[0], "cd")){// Handle built-in command "cd" 
                        status = handle_cd(myCommand);
                        print_complete_message(cmd, status);
                }else if(!strcmp(myCommand->args[0], "pwd")){// Handle built-in command "pwd" 
                        status = handle_pwd();
                        print_complete_message(cmd, status);
                }else if(!strcmp(myCommand->args[0], "sls")){// Handle built-in command "sls" 
                        status = handle_sls();
                        print_complete_message(cmd, status);
                }else{
                        if(!is_pipe){
                                status = execute_command(myCommand, is_redirection, is_append);// Execute single command                              
                                if (status){
                                        free(cmdCopy);
                                        free(myCommand);
                                        continue;
                                }
                                print_complete_message(cmd, status);    
                        }else{
                                pipestatus = execute_command_pipe(myPipe, is_redirection);// Execute pipeline command
                                print_pipeline_complete_message(cmd, pipestatus, pipe_index); 
                        }    
                }

                free(cmdCopy);
                free(myCommand);
        }

        return EXIT_SUCCESS;
}