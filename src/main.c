#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "input_buffer.h"
#include "initial_statement.h"

void print_prompt(){
    printf("db > ");
}

int main(int argc, char* argv[]){
    (void)argc;
    (void)argv;
    InputBuffer* input_buffer = new_input_buffer();

    print_initial_statement();

    while(true){
        print_prompt();
        read_input(input_buffer);

        if (strcmp(input_buffer->buffer, ".exit") == 0) {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        } else {
            printf("Unrecognized Input Command '%s'. \n", input_buffer->buffer);
        }
    }
}