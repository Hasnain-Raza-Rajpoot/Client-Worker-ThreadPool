#include "queue.h"
#include <string.h>

// Print function for integers
void print_int(const void* data) {
    printf("%d", *(int*)data);
}

// Print function for strings
void print_string(const void* data) {
    printf("\"%s\"", (char*)data);
}

// Print function for floats
void print_float(const void* data) {
    printf("%.2f", *(float*)data);
}

int main() {
    printf("=== Generic Queue Test Program ===\n\n");
    
    // Test 1: Integer queue
    printf("Test 1: Integer Queue\n");
    printf("====================\n");
    
    Queue* int_queue = queue_init(sizeof(int));
    if (int_queue == NULL) {
        printf("Failed to create integer queue\n");
        return 1;
    }
    
    // Enqueue some integers
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        if (queue_enqueue(int_queue, &values[i])) {
            printf("Enqueued: %d\n", values[i]);
        } else {
            printf("Failed to enqueue: %d\n", values[i]);
        }
    }
    
    printf("Queue size: %zu\n", queue_size(int_queue));
    printf("Is empty: %s\n", queue_is_empty(int_queue) ? "Yes" : "No");
    
    // Peek at front
    int front_value;
    if (queue_peek(int_queue, &front_value)) {
        printf("Front element: %d\n", front_value);
    }
    
    // Print queue contents
    queue_print(int_queue, print_int);
    
    // Dequeue some elements
    printf("\nDequeuing elements:\n");
    int dequeued_value;
    while (!queue_is_empty(int_queue)) {
        if (queue_dequeue(int_queue, &dequeued_value)) {
            printf("Dequeued: %d\n", dequeued_value);
        }
    }
    
    printf("Queue size after dequeue: %zu\n", queue_size(int_queue));
    printf("Is empty after dequeue: %s\n", queue_is_empty(int_queue) ? "Yes" : "No");
    
    queue_destroy(int_queue);
    printf("\n");
    
    // Test 2: String queue
    printf("Test 2: String Queue\n");
    printf("====================\n");
    
    Queue* str_queue = queue_init(sizeof(char*));
    if (str_queue == NULL) {
        printf("Failed to create string queue\n");
        return 1;
    }
    
    char* strings[] = {"Hello", "World", "Queue", "Test", "Generic"};
    for (int i = 0; i < 5; i++) {
        if (queue_enqueue(str_queue, &strings[i])) {
            printf("Enqueued: \"%s\"\n", strings[i]);
        }
    }
    
    queue_print(str_queue, print_string);
    
    // Dequeue all strings
    char* dequeued_string;
    printf("\nDequeuing all strings:\n");
    while (!queue_is_empty(str_queue)) {
        if (queue_dequeue(str_queue, &dequeued_string)) {
            printf("Dequeued: \"%s\"\n", dequeued_string);
        }
    }
    
    queue_destroy(str_queue);
    printf("\n");
    
    // Test 3: Float queue
    printf("Test 3: Float Queue\n");
    printf("===================\n");
    
    Queue* float_queue = queue_init(sizeof(float));
    if (float_queue == NULL) {
        printf("Failed to create float queue\n");
        return 1;
    }
    
    float floats[] = {3.14f, 2.71f, 1.41f, 1.73f};
    for (int i = 0; i < 4; i++) {
        if (queue_enqueue(float_queue, &floats[i])) {
            printf("Enqueued: %.2f\n", floats[i]);
        }
    }
    
    queue_print(float_queue, print_float);
    
    // Test partial dequeue
    float dequeued_float;
    printf("\nDequeuing first 2 elements:\n");
    for (int i = 0; i < 2; i++) {
        if (queue_dequeue(float_queue, &dequeued_float)) {
            printf("Dequeued: %.2f\n", dequeued_float);
        }
    }
    
    printf("Remaining queue size: %zu\n", queue_size(float_queue));
    queue_print(float_queue, print_float);
    
    queue_destroy(float_queue);
    
    // Test 4: Error handling
    printf("\nTest 4: Error Handling\n");
    printf("======================\n");
    
    Queue* test_queue = queue_init(sizeof(int));
    
    // Test dequeue on empty queue
    int dummy;
    printf("Dequeue from empty queue: %s\n", 
           queue_dequeue(test_queue, &dummy) ? "Success" : "Failed (expected)");
    
    // Test peek on empty queue
    printf("Peek at empty queue: %s\n", 
           queue_peek(test_queue, &dummy) ? "Success" : "Failed (expected)");
    
    // Test enqueue with NULL data
    printf("Enqueue NULL data: %s\n", 
           queue_enqueue(test_queue, NULL) ? "Success" : "Failed (expected)");
    
    queue_destroy(test_queue);
    
    printf("\n=== All tests completed ===\n");
    return 0;
}
