#/**********************************************************
 *Program: 	Small Shell
 *Author:	Andrew Derringer
 *Last Mod:	11/19/2019
 *Summary:	This program implements a terminal shell over
 *		the exisiting architecture. Built in commands
 *		that do not require assistance from forked
 *		processes are handled in main. Those related
 *		to background processing or i/o are modularized
 *		in helper files called by main.
**********************************************************/

/****************************
 * Standard Libraries
****************************/
#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>


/*******************************************
 * Summary: 	Function from previous assignments
 * 		the reads input and places it into a
 * 		string.
 * Param:	Empty string array
 * Param:	int max size of the input buffer
 * Output:	Fills buffer iwth valid input.
*******************************************/
int validInput(char* input_buffer, int max_size) {

   //clear the buffer before accepting new input each turn
   memset(input_buffer, 0, max_size);
   fgets(input_buffer, max_size, stdin); 

   // check if character were dropped by looking for newline
   if (input_buffer[strlen(input_buffer) - 1] != '\n') {
      int dropped = 0;
      while (fgetc(stdin) != '\n') {
         dropped++;
      }

      // exceeded then print and let game loop in main reprint board
      if (dropped > 0) {
         printf("\nYour input exceeded buffer space! Lets try this again.\n");
         return 0;
      }else {
         input_buffer[strlen(input_buffer) - 1] = '\0';
      }
   }

   // grab only char before newline for comparison
   input_buffer[strcspn(input_buffer, "\n")] = 0;

   return 1;
}

/*******************************************
 * Summary: 	Receives a buffer and uses strtok
 * 		to convert it to an array
 * Param:	Filled sting buffer with valid input
 * ParamL	Empty array of strings
 * Output:	Array of input commands from command line
 * 		and returns size of array created
*******************************************/
int string_to_arr(char* input_buffer, char** stringArr) {

   int i = 0;
   char* temp;

   // string token through string buffer delimited by spaces
   char* token = strtok(input_buffer, " ");
   while (token != NULL) {
      stringArr[i] = malloc(strlen(token) + 1);
      strcpy(stringArr[i], token);
      token = strtok(NULL, " ");
      i++;
   }

   // if we reach the max then something is wrong
   if (i >= 512) {
      printf("Too many arguments passed. Let's try this again.\n");
      fflush(stdout);
      free_string_arr(stringArr, i);
      return 0;
   }

   return i;
}

/*******************************************
 * Summary: 	Returns an array of strings to
 * 		empty state
 * Param:	filled array of strings
 * Param:	Size of filled array
 * Output:	Empty or cleared array
*******************************************/
void free_string_arr(char** stringArr, int size) {
   int i;
   for (i = 0; i < size; i++) {
      // Avoiding freeing something that doesn't need it
      if (stringArr[i] != NULL) {
         free(stringArr[i]);
         stringArr[i] = NULL;
      }
   }
}

/*******************************************
 * Summary: 	Prints contents of an array if strings
 * Param:	filled array of strings
 * Param:	Size of filled array
 * Output:	prints array to screen
*******************************************/
void print_arr(char** stringArr, int size) {
   int i;
   for (i = 0; i < size; i++) {
      printf("%s\n", stringArr[i]);
   }
}

/*******************************************
 * Summary: 	Change the working directory. Account
 * 		for blank call to cd, relative, direct path
 * Param:	filled array of command line args
 * Param:	Size of filled array
 * Output:	Changes working directory or outputs error
*******************************************/
void nav_chcwd(char** args_arr, int args_size) {
   char cwd[2048];
   char abs_path[] = "/nfs";

   // If only the cd arg was passed then chdir to home
   if (args_size == 1) {
      chdir(getenv("HOME")); //getenv has member HOME containing our home path
   } 
   // if more than one args than we check if the absolute path was part of user input
   else if (args_size == 2) {
      getcwd(cwd, sizeof(cwd));
      cwd[strlen(cwd)] = '/';
      // if user enter absolute path
      if (strstr(args_arr[1], abs_path) != NULL) {
         if (chdir(args_arr[1]) == -1) {
            printf("Error: Not a valid directory\n");
         }
      } 
      // if not a full absolute path then look for enter dir in current directory
      else {
         strcat(cwd, args_arr[1]);
         if (chdir(cwd) == -1) {
            printf("Error: Not a valid Directory\n");
         }
      }
      memset(cwd, 0, sizeof(cwd));
   } else {
      printf("Error: Too many argument passed. Expected 0 or 1.\n");
   }
}

/*******************************************
 * Summary: 	Looks for $$ in input buffer and expands
 * 		that out to the process pid
 * Param:	filled string buffer of user input
 * Param:	Int pid of current process
 * Output:	Changes the buffer to contain pid where $$ was
*******************************************/
void expand_pid(char* input_buffer, char* symbol, int pid) {
   char temp[2048];
   char* p;

   // convert pid to string
   char pid_str[10];
   sprintf(pid_str, "%d", pid);
 
   // only run if symbol is in buffer
   while(p = strstr(input_buffer, symbol)) {
 
      // cpy everything before the symbol into temp buffer 
      strncpy(temp, input_buffer, p-input_buffer); // Copy characters from 'str' start to 'orig' st$
      temp[p-input_buffer] = '\0';

      // apply the pid string and any remaining original string after symbol to temp
      sprintf(temp+(p-input_buffer), "%s%s", pid_str, p+strlen(symbol));

      memset(input_buffer, 0, 2048);
      strcpy(input_buffer, temp);
      memset(temp, 0, 2049);
   }
}

/*******************************************
 * Summary: 	spawn a fork, change it's signal handling,
 * 		account for errors, and send variables to
 * 		method that calls exec.
 * Param:	filled array of command line args
 * Param:	Size of filled array
 * Param:	Array of background process pids
 * ParamL	Index for next add of a background process
 * Param:	String fg or bg for process type
 * Output:	Creates fork process and calls for
 * 		command processing within. Return for
 * 		fg commands can be captured from call
*******************************************/
int fork_router(char** args_arr, int* args_size, int* bg_pids, int* bg_count, char* process) {
   // prepare new signal handlers for child processes
   struct sigaction ignore_action = {0}, SIGINT_action = {0};
   ignore_action.sa_handler = SIG_IGN;
   SIGINT_action.sa_handler = SIG_DFL;

   int status = 0;
   int exit_method = -5;
   int wait_status;

   // fork process
   int child_pid = fork();
   switch(child_pid) {
      // if fork failed
      case -1:
         perror("Error: Child couldn't fork.\n");
         exit(1);
         break;

      // only run by the child
      case 0:
         // remove the AMP so that it's not applied to exec
         if(strcmp(args_arr[(*args_size) - 1], "&") == 0) {
            free(args_arr[(*args_size) - 1]);
            args_arr[(*args_size) - 1] = NULL;
            (*args_size)--; // decrement arg size after removal
         }

         // if a bg process we want to ignore TSTP, INT
         // CHLD included though program should end before it matters
         if (strcmp(process, "bg") == 0) {
            sigaction(SIGTSTP, &ignore_action, NULL);
            sigaction(SIGCHLD, &ignore_action, NULL);
            sigaction(SIGINT, &ignore_action, NULL);
         
            // required print statement for bg processes
            printf("background pid is %d\n", getpid());
            fflush(stdout);
         } 
         // if a fg process we want to ignore TSTO and CHLD but not INT
         else {
            sigaction(SIGTSTP, &ignore_action, NULL);
            sigaction(SIGCHLD, &ignore_action, NULL);
            sigaction(SIGINT, &SIGINT_action, NULL);
         }
         
         //send relavent arguments for exec
         fork_exec(args_arr, (*args_size), process);
         exit(1);
         break;

      // only run by the parent
      default:
         if (strcmp(process, "fg") == 0) {
            wait_status = waitpid(child_pid, &exit_method, 0);
            // fg processes return their exit status
            return exit_status(wait_status, exit_method);
         } else {
            sleep(1);
            bg_pids[(*bg_count)] = child_pid;
            (*bg_count)++;
            if ((*bg_count) >= 20) {
               (*bg_count) = 0;
            }
         }
         break;
   }
   // bg processes just return 0 not captured
   return 0; 
} 

/*******************************************
 * Summary: 	Uses pid and exit method from a waitpid call
 * 		to print out status information
 * Param:	Int pid if waitpid success or error response
 * Param:	exit method of pid being waited on
 * Output:	Prints finish information of process
*******************************************/
int exit_status(int wait_pid, int exit_method) {
   if (wait_pid == -1) {
      perror("Wait failed...\n");
      exit(1);
   }

   if (WIFEXITED(exit_method)) {
      int exit_status = WEXITSTATUS(exit_method);
      return exit_status;
   }

   if (WIFSIGNALED(exit_method)) {
      int term_sig = WTERMSIG(exit_method);
      printf("\nterminated by signal %d\n");
   }

   return 0;
}

/*******************************************
 * Summary: 	Determines necessary input and output
 * 		redirection and passes commands to exec
 * Param:	filled array of commands fom line
 * Param:	Size of filled array
 * Param:	desired process either fg or bg
 * Output:	Changes i/o and calls exec 
*******************************************/
void fork_exec(char** args_arr, int args_size, char* process) {


   // set bg process i/o to /dev/null
   // Process is open /dev/null assigned to FD
   // Check for error and ask it to close on exec
   int result, inFD, outFD;
   if (strcmp(process, "bg") == 0) {
      // open file to FDs and prep close
      inFD = open("/dev/null", O_RDONLY);
      if (inFD == -1) { perror("open() in\n"); exit(1); }
      fcntl(inFD, F_SETFD, FD_CLOEXEC);
      outFD = open("/dev/null", O_WRONLY);
      if (outFD == -1) { perror("open() out\n"); exit(1); }
      fcntl(outFD, F_SETFD, FD_CLOEXEC); 

      // use dup2 to reasign 0 = stdin to inFD and for outFD
      result = dup2(inFD, 0);
      if (result == -1) { perror("dup2 stdin\n"); exit(1); }
      result = dup2(outFD, 1);
      if (result == -1) { perror("dup2 stdout\n"); exit(1); }
   }

   char* input = NULL;
   char* output = NULL; 
   char* cmdline[512] = {NULL};
   int j = 0;
   int i;
   // loops through args in array and assigns for either input,
   // output, or standard command
   for (i = 0; i < args_size; i++) {
      // if > then the next command in an output, double iterate to skip
      if (strcmp(args_arr[i], ">") == 0 && args_size > i + 1) {
         output = strdup(args_arr[i + 1]);
         i++;
      }
      // if < then the next command is input, double iterate to skip
      else if (strcmp(args_arr[i], "<") == 0 && args_size > i + 1) {
         input = strdup(args_arr[i + 1]);
         i++; 
      }
      // else its a command that can go to exec
      else {
         cmdline[j] = strdup(args_arr[i]);
         j++; 
      }

      // if we caught either new input or output then we manage possible
      // previous open calls by closing and redirect out i/o to these new files.
      if (input != NULL) {
         if (strcmp(process, "bg") == 0) { close(inFD); }
         inFD = open(input, O_RDONLY);
         if (inFD == -1) { printf("cannot open %s for input\n", input); fflush(stdout); exit(1); }
         fcntl(inFD, F_SETFD, FD_CLOEXEC);
         result = dup2(inFD, 0);
         if (result == -1) { perror("dup2 stdin\n"); exit(1); }
      }
      if (output != NULL) {
         if (strcmp(process, "bg") == 0) { close(outFD); }
         outFD = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
         if (outFD == -1) { perror("open() out\n"); exit(1); }
         fcntl(outFD, F_SETFD, FD_CLOEXEC); 
         result = dup2(outFD, 1);
         if (result == -1) { perror("dup2 stdout\n"); exit(1); }
      }
   }

   free(output);
   free(input);

   if (execvp(cmdline[0], cmdline)) {
      printf("%s: no such file or directory\n", cmdline[0]);
      fflush(stdout);
   }

}
