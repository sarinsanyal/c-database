#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <stdio.h>
#include <sys/types.h>

//blueprint for the Inputbuffer struct that
//provides a certain structure to the input text

typedef struct{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer();

void read_input(InputBuffer* input_buffer);
void close_input_buffer(InputBuffer* input_buffer);

#endif