# ECS 150 Project 1 (Simple Shell)
Yang Luo, 920918696 

Yiqiao Lin, 921433714


## Purpose
The purpose of our goal to achieve a shell program is to provide a simple command-line interface, 
allowing users to execute commands, including built-in commands like "cd," "pwd," "sls," and 
external commands, output redirection, pipeline commands, and extra features. It is designed to 
mimic a Unix shell, providing a familiar environment for users to interact with their system.

## Design Choices
1. Command Structure
The program defines a Command structure to represent individual commands entered by the user.
Each command has a command name (cmd), an array of arguments (args), and an optional output file
(outputFile). The structure facilitates the parsing and execution of commands.

2. Input/Output Redirection
To support output redirection, the program defines the redirection function, which detects the
'>' signal, and the append function, which detects the '>>' signal. These functions enable the
redirection of standard output to a specified file. The initCommand_redirection function
initializes a command with redirection. 

3. Command Pipeline
The shell supports command pipelines using the '|' signal. The pipeline function detects the
pipeline signal. Although the initCommand_pipeline function is present, its implementation is not
completed.

4. Execution Mechanism
The program uses fork, wait, and execvp to execute commands in a child process.
The executeCommand function handles the execution of a single command, considering output redirection
and append options. The executeCommand_pipe function is intended for handling pipeline commands,
although its implementation is incomplete.

5. Built-in Commands
The shell includes built-in commands such as "cd," "pwd," "sls," and "exit." These commands are handled
separately from external commands, providing functionality for navigation, listing files, and exiting the
shell.

6. User Interface
The shell provides a simple command prompt (sshell@ucd$) where users can enter commands.
