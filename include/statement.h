#ifndef STATEMENT_H
#define STATEMENT_H

#include "input_buffer.h"
#include "table.h"

typedef enum { 
    EXECUTE_SUCCESS, 
    EXECUTE_DUPLICATE_KEY,
    EXECUTE_TABLE_FULL 
} ExecuteResult;

typedef enum { 
    PREPARE_SUCCESS, 
    PREPARE_SYNTAX_ERROR,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_UNRECOGNIZED_STATEMENT 
} PrepareResult;

// all types of commands in here (SELECT, INSERT, etc.)
typedef enum { 
    STATEMENT_INSERT, 
    STATEMENT_SELECT 
} StatementType;

typedef struct {
    StatementType type;
    Row row_to_insert; //only by insert statement
} Statement;


PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);

ExecuteResult execute_insert(Statement* Statement, Table* table);
ExecuteResult execute_select(Statement* Statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);

#endif