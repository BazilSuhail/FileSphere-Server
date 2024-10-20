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

void writer_entry() {
    pthread_mutex_lock(&mutex);
    
    writers_waiting++;
    // Wait if a reader or another writer is active
    while (readers_count > 0 || writer_active) {
        pthread_cond_wait(&writers_cond, &mutex);
    }
    
    writers_waiting--;
    writer_active = 1;
    pthread_mutex_unlock(&mutex);
}
void writer_exit() {
    pthread_mutex_lock(&mutex);
    
    writer_active = 0;
    
    // Prioritize writers over readers
    if (writers_waiting > 0) {
        pthread_cond_signal(&writers_cond);
    } else {
        pthread_cond_broadcast(&readers_cond);  // Wake up all waiting readers
    }
    
    pthread_mutex_unlock(&mutex);
}
