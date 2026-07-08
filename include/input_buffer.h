#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <stdio.h>
#include <sys/types.h>

#include "table.h"
//blueprint for the Inputbuffer struct that
//provides a certain structure to the input text

typedef struct{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

InputBuffer* new_input_buffer();

void read_input(InputBuffer* input_buffer);
void close_input_buffer(InputBuffer* input_buffer);

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);

#endif