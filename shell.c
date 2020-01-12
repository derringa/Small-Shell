/**********************************************************
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
#include <unistd.h> 
#include <sys/stat.h>
#include <sys/types.h> 
#include <string.h> 
#include <signal.h>


/***************************
 * Program function library
****************************/
#include "helpers.h"


/****************************
 * Global Constants
****************************/
#define MAX_LINE 2048
#define MAX_ARR 512
#define MAX_BG 20


/****************************
 * Global Variables
****************************/
int useAMP = 1;
pid_t bg_pids[MAX_BG] = {0};
int bg_count = 0;


/*******************************************
 * Summary: 	Custom signal catcher for SIGSTP. It
 * 		triggers a bool for whether to ignore
 * 		the & command that allows for background
 * 		processes.
 * Param:	System Signal int
 * Output:	Flips useAMP bool
*******************************************/
void catchSIGTSTP(int signo) {
   // if background mode active
   if (useAMP == 1) {
      char* message = "Entering foreground-only mode (& is now ignored)\n:";
      write(STDOUT_FILENO, message, 50); // can't use printf in signal handling
      useAMP = 0;
   } // if background mode inactive
   else {
      char* message = "Exiting foreground-only mode\n:";
      write(STDOUT_FILENO, message, 30);
      useAMP = 1;     
   }
} 

/*******************************************
 * Summary: 	Loops through array of background
 * 		child process pids. Array is init
 * 		to all 0 and after finished processes
 * 		are found that index is reassigned to 0.
 * Param:	Int array containing active background pids
 * Output:	Prints status of completed process and
 * 		manages array.
*******************************************/
void print_bg() {
   int i, exit_method;
   pid_t pid;
   for(i = 0; i < MAX_BG; i++) {
      // empty indexes hold 0
      if (bg_pids[i] > 0) {
         // pid assigned to finished pid or error -1
         pid = waitpid(bg_pids[i], &exit_method, WNOHANG);
         if (pid == -1) {
            perror("Wait failed...\n");
         }
         // if process number returns
         else if (pid > 0) { 
            // check if process exited and get status number if so
            if (WIFEXITED(exit_method)) {
               int exit_status = WEXITSTATUS(exit_method);
               printf("background pid %d is done: exit value %d\n", bg_pids[i], exit_status);
               fflush(stdout);
            } 
            // check if process was signal terminal and get dignal if so
            else if (WIFSIGNALED(exit_method)) {
               int term_sig = WTERMSIG(exit_method);
               printf("background pid %d is done: terminated by signal %d\n", bg_pids[i], term_sig);
               fflush(stdout);
            }
            // reset index to 0 for nect check
            bg_pids[i] = 0;
         }
      }
   }
}

int main() {

   // declare empty sigaction struct
   struct sigaction SIGTSTP_action = {0}, ignore_action = {0};
   // assign relevant member functions
   SIGTSTP_action.sa_handler = catchSIGTSTP;
   sigfillset(&SIGTSTP_action.sa_mask);
   SIGTSTP_action.sa_flags = SA_RESTART;

   ignore_action.sa_handler = SIG_IGN;
   //associate specific signals with specified structd
   sigaction(SIGTSTP, &SIGTSTP_action, NULL);
   sigaction(SIGINT, &ignore_action, NULL);

   int exit = 0;
   int status = 0;
   char input_buffer[MAX_LINE];
   char* args_arr[MAX_ARR] = { NULL };
   int args_size;

   
   // continue while exit hasn't been entered
   while(exit == 0) {

      // print any finished background processes before allowig input
      print_bg();
      printf(":");
      fflush(stdout);

      // put input in buffer expand && using getpid and get number of args passed
      if (validInput(input_buffer, MAX_LINE) == 1) {
         expand_pid(input_buffer, "$$", getpid());
         args_size = string_to_arr(input_buffer, args_arr);      
      }

      // if nothing printed loop back to input
      if (args_size > 0) {
         // exit causes bool to switch and program to end
         if(strcmp(args_arr[0], "exit") == 0 && args_size == 1) {
            exit = 1;
         } 
         // if cd is first arg passed use custom nav function
         else if (strcmp(args_arr[0], "cd") == 0) {
            nav_chcwd(args_arr, args_size);
         } 
         // if status print the last status report from foreground process
         else if (strcmp(args_arr[0], "status") == 0) {
            printf("exit value %d\n", status);
            fflush(stdout);
         } 
         // if & passed at end and mode allows background then run bg else fg
         else if (args_arr[0][0] != '#' && args_size > 0) {
            if (strcmp(args_arr[args_size - 1], "&") == 0 && useAMP == 1) {
               fork_router(args_arr, &args_size, bg_pids, &bg_count, "bg");
            } else {
               status = fork_router(args_arr, &args_size, bg_pids, &bg_count, "fg");
            }
         }
      }
      // empty the string to the next input
      free_string_arr(args_arr, args_size);
   }

   printf("Terminating...\n");

   return 0;
}
