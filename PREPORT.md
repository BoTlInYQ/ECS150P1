# ECS 150 Project 1 (Simple Shell)
Yang Luo, 920918696 

Yiqiao Lin, 921433714


## Purpose
The purpose of our goal to achieve a shell program is to provide a simple command-line interface, 
allowing users to execute commands, including built-in commands like "cd," "pwd," "sls," and 
external commands, output redirection, pipeline commands, and extra features. It is designed to 
mimic a Unix shell, providing a familiar environment for users to interact with their system.

## Design Choices
1. Structure
The program defines a Command structure to represent individual commands entered by the user.
Each command has a command name (cmd), an array of arguments (args), and an optional output file
(outputFile). The structure facilitates the parsing and execution of commands.
Also the program defines a Pipe structure that contain four Command Structure.

2. Output Redirection
To support output redirection, the program defines the redirection function, which detects the
```>``` signal, and the append function, which detects the ```>>``` signal. These functions enable the
redirection of standard output to a specified file. The initCommand_redirection function
initializes a command with redirection. 

3. Command Pipeline
The shell supports command pipelines using the '|' signal. The pipeline function detects the
pipeline signal. We try to separate pipeline command by '|' and check if the right side have another '|' for another separation.

4. Execution Mechanism
The program uses ```fork```, ```wait```, and ```execvp``` to execute commands in a child process.
The executeCommand function handles the execution of a single command, considering output redirection
and append options. The executeCommand_pipe function is intended for handling pipeline commands,
although its implementation is incomplete.

5. Built-in Commands
The shell includes built-in commands such as ```cd```, ```pwd```, ```exit```, and ```sls``` 
These commands are handled separately from external commands, providing functionality for navigation, listing files, and exiting the
shell.

6. User Interface
The shell provides a simple command prompt (sshell@ucd$) where users can enter commands.

7. Implement direction
Separate the shell implementation into two parts, one without pipeline and one with pipeline. After separate command when pipeline signal detect, reuse the single command code for each fragment.

## Execution Procedure
Command Input:

The program prompts the user with the shell prompt (sshell@ucd$).
User inputs a command.
Command Parsing:

The program parses the entered command, checking for special signals like  ```>```, ```>>```, and ``|``.
The appropriate initCommand function is called based on the presence of redirection, append, or pipeline.
Command Execution:

For non-pipeline commands, the program forks a child process, redirects output if needed, and executes the command using ```execvp```.
For pipeline commands (incomplete), the program should set up pipes and execute commands in a sequence.
Built-in Commands:

If the command is a built-in command ("cd," "pwd," "sls," or "exit"), it is handled separately, and no child process is created.
Completion Message:

After command execution, the program prints a completion message, indicating the status and the command executed.
Repeat:

Steps 1-5 are repeated until the user enters the "exit" command.

## Test
Self testing for each part of program, also use "tester.sh" to detect program.
Even though we aren't able to pass pipe test, but we passed every other test.
Using the test case in the Project script is quite useful for knowing what message should be print, like when "Error: command not found". it also print "+ completed 'command' [1]". We add the missing completation message.

## Source
Everything included in lecture slide, project script(including the external links).
