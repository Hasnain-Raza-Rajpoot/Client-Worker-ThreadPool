#include "queue.h"


Queue* queue_init(size_t data_size) {
    if (data_size == 0) {
        return NULL;
    }
    
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }
    
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    queue->data_size = data_size;
    
    return queue;
}

bool queue_enqueue(Queue* queue, const void* data) {
    if (queue == NULL || data == NULL) {
        return false;
    }
    
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    if (new_node == NULL) {
        return false;
    }
    
    new_node->data = malloc(queue->data_size);
    if (new_node->data == NULL) {
        free(new_node);
        return false;
    }
    
    // Copy data to the new node
    memcpy(new_node->data, data, queue->data_size);
    new_node->next = NULL;
    
    if (queue->rear == NULL) {
        // First element
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        // Add to rear
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
    
    queue->size++;
    return true;
}

bool queue_dequeue(Queue* queue, void* data) {
    if (queue == NULL || queue_is_empty(queue)) {
        return false;
    }
    
    QueueNode* temp = queue->front;
    
    if (data != NULL) {
        memcpy(data, temp->data, queue->data_size);
    }
    
    queue->front = queue->front->next;
    
    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    
    free(temp->data);
    free(temp);
    queue->size--;
    
    return true;
}

bool queue_peek(const Queue* queue, void* data) {
    if (queue == NULL || queue_is_empty(queue) || data == NULL) {
        return false;
    }
    
    memcpy(data, queue->front->data, queue->data_size);
    return true;
}

size_t queue_size(const Queue* queue) {
    if (queue == NULL) {
        return 0;
    }
    return queue->size;
}

bool queue_is_empty(const Queue* queue) {
    return (queue == NULL || queue->front == NULL);
}

void queue_destroy(Queue* queue) {
    if (queue == NULL) {
        return;
    }
    
    QueueNode* current = queue->front;
    while (current != NULL) {
        QueueNode* temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    
    free(queue);
}

void queue_print(const Queue* queue, void (*print_func)(const void*)) {
    if (queue == NULL || print_func == NULL) {
        printf("Queue is NULL or print function is NULL\n");
        return;
    }
    
    if (queue_is_empty(queue)) {
        printf("Queue is empty\n");
        return;
    }
    
    printf("Queue contents (size: %zu): ", queue->size);
    QueueNode* current = queue->front;
    while (current != NULL) {
        print_func(current->data);
        if (current->next != NULL) {
            printf(" -> ");
        }
        current = current->next;
    }
    printf("\n");
}
