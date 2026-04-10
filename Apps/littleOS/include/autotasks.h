// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
#ifndef AUTOTASKS_H
#define AUTOTASKS_H

typedef struct {
    const char* cmd_string;
    const char* display_name;
    void (*entrypoint)(void*);
    int* id_ptr;
} TaskRegistry;

extern TaskRegistry autotasks[7];
extern const int numAutotasks;


void init_all_tasks(void);

#endif
