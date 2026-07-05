#include <stdio.h>
#include <stdbool.h>
#include "input_buffer.h"
#include "initial_statement.h"

int main(int argc, char* argv[]){
    (void)argc;
    (void)argv;

    print_initial_statement();
    // InputBuffer* input_buffer = new_input_buffer();

    while(true){
        printf("db> ");
        printf("\n");

        //take input here

        break;
    }

    return 0;
}