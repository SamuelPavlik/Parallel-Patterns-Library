#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "Pipeline.h"
#include "Farm.h"

#define MAX_NUM 10
#define FIB_NUM 100000

/**
 * Add stage to pipeline
 * @param pipeline Pipeline struct pointer
 * @param stageData Pointer to base Stage struct of this stage
 */
void Pipe_addStage(Pipeline *pipeline, Stage* stageData) {
    if (strcmp(stageData->name, PIPE) != 0) {
        //assign input queue
        if (pipeline->size > 0) {
            stageData->input = pipeline->stages[pipeline->size - 1]->output;
        }
        else {
            stageData->input = Queue_init();
            pipeline->base.input = stageData->input;
        }

        stageData->output = Queue_init();
        pipeline->base.output = stageData->output;
        pipeline->stages[pipeline->size] = stageData;
        pipeline->size = pipeline->size + 1;

        //if farm, assign input and output queue to each worker
        if (strcmp(stageData->name, FARM) == 0) {
            Farm_setWorkers((Farm *) stageData);
        }
    }
    else {
        Pipeline* stagePipe = ((Pipeline *) stageData);
        for (int i = 0; i < stagePipe->size; ++i) {
            Pipe_addStage(pipeline, stagePipe->stages[i]);
        }
    }
}

/**
 * Put data into pipeline input queue
 * @param pipeline pipeline struct pointer
 * @param data array of data pointers
 * @param size size of the array
 */
void Pipe_putToQueue(Pipeline* pipeline, void* data[], int size) {
    for (int i = 0; i < size; ++i) {
        QueueNode* node = malloc(sizeof(QueueNode));
        node->data = data[i];
        Queue_put(node, pipeline->base.input);
    }
    QueueNode* node = malloc(sizeof(QueueNode));
    int* eos = malloc(sizeof(int));
    *eos = EOS;
    node->data = eos;
    Queue_put(node, pipeline->base.input);
}

/**
 * Get data in output queue to data array
 * @param pipeline pipeline struct pointer
 * @return the data array
 */
void** Pipe_getOutput(Pipeline *pipeline) {
    int size = pipeline->base.output->size;
    void** data = malloc(size * sizeof(void*));
    for (int i = 0; i < size; ++i) {
        QueueNode* node = Queue_get(pipeline->base.output);
        data[i] = node->data;
        free(node);
    }

    return data;
}

/**
 * Run pipeline
 * @param pipeline pipeline struct pointer
 */
void Pipe_runPipe(Pipeline *pipeline) {
    for (int i = 0; i < pipeline->size; ++i) {
        Stage* stage = pipeline->stages[i];
        if (strcmp(stage->name, FARM) == 0) {
            Farm* farm = (Farm *) stage;
            Farm_run(farm);
            Farm_collect(farm);
        }
        else if (strcmp(stage->name, STAGE) == 0) {
            Worker* worker = (Worker *) stage;
            Worker_run(worker);
            Worker_collect(worker);
        }
        else if (strcmp(stage->name, PIPE) == 0) {
            Pipeline* pipe = ((Pipeline *) stage);
            Pipe_runPipe(pipe);
        }
        else {
            printf("Unknown stage type: %s\n", stage->name);
            exit(1);
        }
    }
}

/**
 * Measure the time it takes to execute a pipeline in miliseconds
 * @param pipeline pipeline struct pointer
 * @return time in miliseconds
 */
long long Pipe_measure(Pipeline* pipeline) {
    struct timeval te;
    double dif;
    gettimeofday(&te, NULL);
    long long start = te.tv_sec*1000LL + te.tv_usec/1000;

    Pipe_runPipe(pipeline);

    gettimeofday(&te, NULL);
    long long end = te.tv_sec*1000LL + te.tv_usec/1000;

    return end - start;
}

/**
 * The example function calculating elements i Fibonacci sequence
 * @param n index of element in the sequence to calculate
 * @return nth element
 */
int fib(int n)
{
    int  i, Fnew, Fold, temp,ans;

    Fnew = 1;  Fold = 0;
    for ( i = 2;
          i <= n;          /* apsim_loop 1 0 */
          i++ )
    {
        temp = Fnew;
        Fnew = Fnew + Fold;
        Fold = temp;
    }
    ans = Fnew;
    return ans;
}

/**
 * The Fibonacci function wrapped in a function that can be passed to a stage initializer
 * @param n index of element in the sequence to calculate
 * @return nth element
 */
void* fibFunc(void* n) {
    *((int *) n) = fib(*((int *) n));
    return n;
}

/**
 * Measure time it takes farm pattern to finish a number of tasks. The complexity of tasks raises exponentially.
 * @param numOfWorkers Number of workers in the farm
 * @param maxIndex Number of tasks to finish.
 * @return Array of times, where each time corresponds to a different task.
 */
long long* Pipe_measureFarm(int numOfWorkers, int maxIndex){
    long long* times = malloc(maxIndex * sizeof(long long));
    Pipeline* pipeline = Pipe_init();
    Farm* farm = Farm_init(numOfWorkers, fibFunc);
    Pipe_addStage(pipeline, (Stage *) farm);
    int size = 1;

    for (int i = 0; i < maxIndex; i++) {
        void* data[size];
        for (int j = 0; j < size; ++j) {
            int* num = malloc(sizeof(int));
            *num = FIB_NUM;
            data[j] = num;
        }
        printf("%d\n", size);
        Pipe_putToQueue(pipeline, data, size);

        times[i] = Pipe_measure(pipeline);

        //cleanup
        void** output = Pipe_getOutput(pipeline);
        for (int j = 0; j < size; ++j) {
            free(output[j]);
        }
        free(output);
        size*=2;
    }

    Pipe_destroy(pipeline);
//    Farm_destroy(farm);
    printf("\n");

    return times;
}

/**
 * Initialize pipeline struct
 * @return pipeline struct pointer
 */
Pipeline* Pipe_init() {
    Pipeline* pipeline = malloc(sizeof(Pipeline));
    pipeline->size = 0;
    pipeline->stages = malloc(MAX_NUM * sizeof(Stage*));
    pipeline->base.name = PIPE;

    return pipeline;
}

/**
 * Destroy pipeline struct
 * @param pipeline pipeline struct pointer
 */
void Pipe_destroy(Pipeline* pipeline) {

    for (int i = 0; i < pipeline->size; ++i) {
        Stage* stage = pipeline->stages[i];

        if (strcmp(stage->name, FARM) == 0) {
            Farm* farm = (Farm *) stage;
            Farm_destroy(farm);
        }
        else if (strcmp(stage->name, STAGE) == 0) {
            Worker* worker = (Worker *) stage;
            Worker_destroy(worker);
        }
        else if (strcmp(stage->name, PIPE) == 0) {
            Pipeline* pipe = ((Pipeline *) stage);
            Pipe_destroy(pipe);
        }
        else {
            printf("Unknown stage type: %s\n", stage->name);
            exit(1);
        }
    }

    Queue_destroy(pipeline->base.output);
    free(pipeline->stages);
    free(pipeline);
}

/**
 * Print info about this pipe
 * @param this pipeline struct pointer
 */
void Pipe_print(Pipeline* this) {
    printf("Pipeline %p\n", this);
    printf("Input: %p\n", this->base.input);
    printf("Output: %p\n", this->base.output);
    printf("Stages %d\n", this->size);
    for (int i = 0; i < this->size; ++i) {
        printf("-Stage %s %p\n", this->stages[i]->name, this->stages[i]);
        if (strcmp(this->stages[i]->name, FARM) == 0) {
            Farm_print((Farm *) this->stages[i]);
        }
        else if (strcmp(this->stages[i]->name, PIPE) == 0) {
            Pipe_print((Pipeline *) this->stages[i]);
        }
        else {
            printf("  -Input: %p\n", this->stages[i]->input);
            printf("  -Output: %p\n", this->stages[i]->output);

        }
    }
}