#include <stdio.h>
#include <unistd.h>
#include "initial_statement.h"

void print_initial_statement() {
    fflush(stdout); 
    
    //Pause exec for 1s
    sleep(1);
    
    printf("db version 0.1 2026\n");
    printf("Enter \".help\" for usage hints.\n");
    printf("Connected to a transient in-memory database.\n\n");

    sleep(1);
}