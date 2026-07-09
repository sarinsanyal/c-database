#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "table.h"

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100

typedef struct {
    uint32_t id; 
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1]; 
} Row;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    Pager* pager;
    uint32_t num_rows;
} Table;

typedef struct{
    Table* table;
    uint32_t row_num;
    bool end_of_table; //indicates a portion one past the last element -> possibly to insert a new row
} Cursor;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

extern const uint32_t ID_SIZE;
extern const uint32_t USERNAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t USERNAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;
extern const uint32_t PAGE_SIZE;
extern const uint32_t ROWS_PER_PAGE;
extern const uint32_t TABLE_MAX_ROWS;

Table* db_open(const char* filename);
Pager* pager_open(const char*filename);

Cursor* table_start(Table* table);
Cursor* table_end(Table* table);

void print_row(Row* row);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void* cursor_value(Cursor* cursor);
void* get_page(Pager* pager, uint32_t page_num);
void cursor_advance(Cursor* cursor);
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
void db_close(Table* table);

#endif