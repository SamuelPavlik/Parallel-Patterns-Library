#ifndef CS4204PRACTICAL2_WORKER_H
#define CS4204PRACTICAL2_WORKER_H

#include <pthread.h>
#include "Stage.h"

#define FINISHED 0
#define PROCESSING 1
#define WAITING 2

typedef struct {
    Stage base;
//    char* name;
//    Queue* input;
//    Queue* output;
//    void* (*func)(void*);
    pthread_t* id;
    int status;
} Worker;

Worker* Worker_init(void* (*func)(void*), Queue* input, Queue* output, pthread_t* id);

void Worker_destroy(Worker* this);

void Worker_setPipes(Worker* this, Queue* input, Queue* output);

void Worker_run(Worker* this);

void Worker_collect(Worker* this);

#endif //CS4204PRACTICAL2_WORKER_H
