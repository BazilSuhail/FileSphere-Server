#include "helper.h"

// Starting reing prcess with queue ased sync
void startRead(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    Request request = { .type = READ };
    pthread_cond_init(&request.cond, NULL);

    enqueueRequest(user, &request);
    while (user->isWriting || user->queueFront != user->queueBack && user->requestQueue[user->queueFront] != &request) {
        pthread_cond_wait(&request.cond, &user->mutex);
    }

    dequeueRequest(user);
    user->readCount++;
    processQueue(user);
    pthread_mutex_unlock(&user->mutex);
}

// Finishing reading process and update queue
void finishRead(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->readCount--;
    if (user->readCount == 0) {
        processQueue(user);
    }
    pthread_mutex_unlock(&user->mutex);
}

// Start wrting process with queue based synch
void startWrite(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    Request request = { .type = WRITE };
    pthread_cond_init(&request.cond, NULL);

    enqueueRequest(user, &request);
    while (user->isWriting || user->readCount > 0 || user->queueFront != user->queueBack && user->requestQueue[user->queueFront] != &request) {
        pthread_cond_wait(&request.cond, &user->mutex);
    }

    dequeueRequest(user);
    user->isWriting = 1;
    pthread_mutex_unlock(&user->mutex);
}

void finishWrite(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->isWriting = 0;
    processQueue(user);
    pthread_mutex_unlock(&user->mutex);
}