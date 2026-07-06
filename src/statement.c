#include <stdio.h>
#include <string.h>

#include "statement.h"

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
    char* query = input_buffer->buffer;

    if (strncmp(query, "insert", 6) == 0 ){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (strcmp(query, "select") == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

//
void execute_statement(Statement* statement){
    switch (statement -> type) {
        case (STATEMENT_INSERT):
            printf("INSERT Command is done here.\n");
            break;
            
        case (STATEMENT_SELECT):
            printf("SELECT Command is done here.\n");
            break;
    }
}   