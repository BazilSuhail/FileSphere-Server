#include "helper.h"

void reader_entry() {
    pthread_mutex_lock(&mutex);
    
    // Wait if a writer is active or waiting
    while (writer_active || writers_waiting > 0) {
        pthread_cond_wait(&readers_cond, &mutex);
    }
    
    readers_count++;
    pthread_mutex_unlock(&mutex);
}

void reader_exit() {
    pthread_mutex_lock(&mutex);
    
    readers_count--;
    if (readers_count == 0) {
        // Signal a writer if waiting
        pthread_cond_signal(&writers_cond);  
    }
    
    pthread_mutex_unlock(&mutex);
}
