#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tree.h"
#include "table.h"

// Node common layout definitions
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = sizeof(uint8_t); 
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = sizeof(uint8_t) + sizeof(uint8_t); 
const uint8_t COMMON_NODE_HEADER_SIZE = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t);

// Leaf Node Layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = 6;
const uint32_t LEAF_NODE_HEADER_SIZE = 6 + sizeof(uint32_t);

const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = 293; 
const uint32_t LEAF_NODE_VALUE_OFFSET = sizeof(uint32_t);
const uint32_t LEAF_NODE_CELL_SIZE = sizeof(uint32_t) + 293; 
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = 4096 - 10; 
const uint32_t LEAF_NODE_MAX_CELLS = (4096 - 10) / (sizeof(uint32_t) + 293);

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - ((LEAF_NODE_MAX_CELLS + 1) / 2);

// Internal Node Layout
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = COMMON_NODE_HEADER_SIZE + sizeof(uint32_t);
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + sizeof(uint32_t) + sizeof(uint32_t);

const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = sizeof(uint32_t) + sizeof(uint32_t);
const uint32_t INTERNAL_NODE_MAX_CELLS = 3; // Keep small for testing splits

// Accessor helpers
uint32_t* leaf_node_num_cells(void* node) { return node + LEAF_NODE_NUM_CELLS_OFFSET; }
void* leaf_node_cell(void* node, uint32_t cell_num) { return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE; }
uint32_t* leaf_node_key(void* node, uint32_t cell_num) { return leaf_node_cell(node, cell_num); }
void* leaf_node_value(void* node, uint32_t cell_num) { return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE; }

uint32_t* internal_node_num_keys(void* node) { 
    return (uint32_t*)((char*)node + INTERNAL_NODE_NUM_KEYS_OFFSET); 
}

uint32_t* internal_node_right_child(void* node) { 
    return (uint32_t*)((char*)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET); 
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num) { 
    return (uint32_t*)((char*)node + INTERNAL_NODE_HEADER_SIZE + (cell_num * INTERNAL_NODE_CELL_SIZE)); 
}

uint32_t* internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    }
    
    if (child_num == num_keys) {
        return internal_node_right_child(node);
    } else {
        return internal_node_cell(node, child_num);
    }
}

uint32_t* internal_node_key(void* node, uint32_t key_num) { 
    // Get the start of the cell, cast to char* to move by EXACTLY 4 bytes, then return uint32_t*
    return (uint32_t*)((char*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE); 
}

uint32_t node_parent(void* node) { return *((uint32_t*)(node + PARENT_POINTER_OFFSET)); }
void set_node_parent(void* node, uint32_t parent_page_num) { *((uint32_t*)(node + PARENT_POINTER_OFFSET)) = parent_page_num; }

NodeType get_node_type(void* node) { return (NodeType)(*((uint8_t*)(node + NODE_TYPE_OFFSET))); }
void set_node_type(void* node, NodeType type) { *((uint8_t*)(node + NODE_TYPE_OFFSET)) = (uint8_t)type; }
bool is_node_root(void* node) { return (bool)(*((uint8_t*)(node + IS_ROOT_OFFSET))); }
void set_node_root(void* node, bool is_root) { *((uint8_t*)(node + IS_ROOT_OFFSET)) = (uint8_t)is_root; }

uint32_t get_unused_page_num(Pager* pager) { return pager->num_pages; }

uint32_t get_node_max_key(void* node) {
    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node) - 1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }
    return 0;
}

void initialize_leaf_node(void* node) { 
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0; 
}

void initialize_internal_node(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

uint32_t internal_node_find_child(void* node, uint32_t key) {
    uint32_t num_keys = *internal_node_num_keys(node);
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;

    while (min_index < max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_check = *internal_node_key(node, index);
        if (key_to_check >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }
    return min_index;
}

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    uint32_t old_page_num = parent_page_num;
    void* old_node = get_page(table->pager, old_page_num);
    uint32_t old_max_key = get_node_max_key(old_node);

    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(child);

    uint32_t new_page_num = get_unused_page_num(table->pager);
    void* new_node = get_page(table->pager, new_page_num);
    initialize_internal_node(new_node);

    uint32_t num_keys = *internal_node_num_keys(old_node);
    
    // Calculate split distribution
    uint32_t old_node_num_keys = num_keys / 2;
    uint32_t new_node_num_keys = num_keys - old_node_num_keys;

    // Save the old rightmost child to migrate to the new node
    *internal_node_right_child(new_node) = *internal_node_right_child(old_node);
    
    // The splitting cell's child becomes the new rightmost child of the old node
    uint32_t* separating_cell = internal_node_cell(old_node, old_node_num_keys);
    *internal_node_right_child(old_node) = *separating_cell;

    // Update internal key totals now that layout boundaries are adjusted
    *internal_node_num_keys(old_node) = old_node_num_keys;
    *internal_node_num_keys(new_node) = new_node_num_keys;

    // FIX: Copy elements starting from old_node_num_keys now that old count has shrunk
    void* destination = internal_node_cell(new_node, 0);
    void* source = internal_node_cell(old_node, old_node_num_keys);
    uint32_t bytes_to_copy = new_node_num_keys * INTERNAL_NODE_CELL_SIZE;
    
    memcpy(destination, source, bytes_to_copy);

    // Insert the new child node into the appropriate split side
    if (child_max_key > get_node_max_key(old_node)) {
        internal_node_insert(table, new_page_num, child_page_num);
    } else {
        internal_node_insert(table, old_page_num, child_page_num);
    }

    // Pass the split up to the root or parent node level
    if (is_node_root(old_node)) {
        create_new_root(table, new_page_num);
    } else {
        uint32_t parent_parent_page_num = node_parent(old_node);
        void* parent_parent = get_page(table->pager, parent_parent_page_num);
        update_internal_node_key(parent_parent, old_max_key, get_node_max_key(old_node));
        internal_node_insert(table, parent_parent_page_num, new_page_num);
    }
}


void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = get_page(table->pager, parent_page_num);
    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t original_num_keys = *internal_node_num_keys(parent);
    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        internal_node_split_and_insert(table, parent_page_num, child_page_num);
        return;
    }

    uint32_t right_child_page_num = *internal_node_right_child(parent);
    void* right_child = get_page(table->pager, right_child_page_num);

    if (child_max_key > get_node_max_key(right_child)) {
        *internal_node_child(parent, original_num_keys) = right_child_page_num;
        *internal_node_key(parent, original_num_keys) = get_node_max_key(right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        for (uint32_t i = original_num_keys; i > index; i--) {
            void* destination = internal_node_cell(parent, i);
            void* source = internal_node_cell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }

    *internal_node_num_keys(parent) = original_num_keys + 1;
    set_node_parent(child, parent_page_num);
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    void* root = get_page(table->pager, table->root_page_num);
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    void* left_child = get_page(table->pager, left_child_page_num);

    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;

    set_node_parent(left_child, table->root_page_num);
    void* right_child = get_page(table->pager, right_child_page_num);
    set_node_parent(right_child, table->root_page_num);
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }

    if (cursor->cell_num < num_cells) {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
    void* old_node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t old_max_key = get_node_max_key(old_node);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    void* new_node = get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);
    set_node_parent(new_node, node_parent(old_node));

    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* destination_node;
        if (i >= (int32_t)LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }
        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        void* destination = leaf_node_cell(destination_node, index_within_node);

        if ((uint32_t)i == cursor->cell_num) {
            serialize_row(value, destination);
            *leaf_node_key(destination_node, index_within_node) = key;
        } else if ((uint32_t)i > cursor->cell_num) {
            memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    if (is_node_root(old_node)) {
        create_new_root(cursor->table, new_page_num);
    } else {
        uint32_t parent_page_num = node_parent(old_node);
        void* parent = get_page(cursor->table->pager, parent_page_num);
        update_internal_node_key(parent, old_max_key, get_node_max_key(old_node));
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
    }
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t child_index = internal_node_find_child(node, key);
    uint32_t child_page_num = *internal_node_child(node, child_index);
    void* child = get_page(table->pager, child_page_num);
    switch (get_node_type(child)) {
        case NODE_LEAF:
            return leaf_node_find(table, child_page_num, key);
        case NODE_INTERNAL:
            return internal_node_find(table, child_page_num, key);
    }
    return NULL;
}

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;

    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if (key == key_at_index) {
            cursor->cell_num = index;
            return cursor;
        }
        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }        
    }
    cursor->cell_num = min_index;
    return cursor;
}

void print_constants() {
    printf("ROW_SIZE: %d\n", LEAF_NODE_VALUE_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void indent(uint32_t level) {
    for (uint32_t i = 0; i < level; i++) {
        printf("  ");
    }
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = get_page(pager, page_num);
    uint32_t num_keys, child;

    switch (get_node_type(node)) {
        case (NODE_LEAF):
            num_keys = *leaf_node_num_cells(node);
            indent(indentation_level);
            if (indentation_level == 0) {
                printf("leaf (size %d)\n", num_keys);
            } else {
                printf("- leaf (size %d)\n", num_keys);
            }
            for (uint32_t i = 0; i < num_keys; i++) {
                indent(indentation_level + 1);
                if (indentation_level == 0) {
                     printf("- %d : %d\n", i, *leaf_node_key(node, i));
                } else {
                     printf("- %d\n", *leaf_node_key(node, i));
                }
            }
            break;
        case (NODE_INTERNAL):
            num_keys = *internal_node_num_keys(node);
            indent(indentation_level);
            printf("- internal (size %d)\n", num_keys);
            
            // Loop through the internal cells (0 to num_keys - 1)
            for (uint32_t i = 0; i < num_keys; i++) {
                // 1. Print the child pointer at this index position
                child = *internal_node_child(node, i);
                print_tree(pager, child, indentation_level + 1);

                // 2. Print the key separating this child from the next
                indent(indentation_level + 1);
                printf("- key %d\n", *internal_node_key(node, i));
            }
            
            // 3. Print the final rightmost child exactly once here
            child = *internal_node_right_child(node);
            print_tree(pager, child, indentation_level + 1);
            break;
    }
}