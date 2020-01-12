# Small-Shell #
A shell program allowing redirection of inputs and outputs, and supporting both foreground and background processes.

## Features ##
Built-In commands include:
```
exit
cd
status
```

Intelligent redirection of inputs and outputs.
Comments by beginning a line with the # character.
User specified background processes by ending a line with &.
Toggle background process functionality on/off by signal handling CTRL-Z (SIGSTP).

Other commands are handled using execcvp()

## Description ##
The colon : symbol is the command line prompt.

The general syntax of a command:
command [arg1 arg2 ...] [< input_file] [> output_file] [&]

### Syntax: ###
* All words or built in commands must be seperated by spaces. This includes <, >, &.

* Backgroun executed commands must end with &.

* Input redirection can appear before or after output redirection.

* Neigther the pipe command | or arguments with spaces surrounded by quotes is supported.

* Handles commands with a maximum length of 2048 characters and 512 arguments.

* No error checking is performed on the syntax of the command line.

## Instructions ##
Compile the run program with:
gcc -o smallsh smallsh.c

Run the shell with:
smallsh

Exit the shell with:
exit
