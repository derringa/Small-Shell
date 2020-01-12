#ifndef HELPERS_H
#define HELPERS_H

int validInput(char*, int);
int string_to_arr(char*, char**);
void free_string_arr(char**, int);
void print_arr(char**, int);
void nav_chcwd(char**, int);
void expand_pid(char*, char*, int); 
int fork_router(char**, int*, int*, int*, char*);
int exit_status(int, int);
void fork_exec(char**, int, char*);
 
#endif
