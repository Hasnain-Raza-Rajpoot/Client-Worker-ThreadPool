#ifndef QUEUE_H
#define QUEUE_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct QueueNode {
    void* data;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
    size_t size;
    size_t data_size;
} Queue;

Queue* queue_init(size_t data_size);
bool queue_enqueue(Queue* queue, const void* data);
bool queue_dequeue(Queue* queue, void* data);
bool queue_peek(const Queue* queue, void* data);
size_t queue_size(const Queue* queue);
bool queue_is_empty(const Queue* queue);
void queue_destroy(Queue* queue);
void queue_print(const Queue* queue, void (*print_func)(const void*));

#endif 
