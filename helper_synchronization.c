#include "helper.h"

// Start reading process with queue-based synchronization
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

// Finish reading process and update queue
void finishRead(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->readCount--;
    if (user->readCount == 0) {
        processQueue(user);
    }
    pthread_mutex_unlock(&user->mutex);
}

// Start writing process with queue-based synchronization
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

// Finish writing process and update queue
void finishWrite(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->isWriting = 0;
    processQueue(user);
    pthread_mutex_unlock(&user->mutex);
}


/*
void startRead(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    while (user->isWriting) {
        pthread_cond_wait(&user->cond, &user->mutex);
    }
    user->readCount++;
    pthread_mutex_unlock(&user->mutex);
}

void finishRead(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->readCount--;
    if (user->readCount == 0) {
        pthread_cond_signal(&user->cond);
    }
    pthread_mutex_unlock(&user->mutex);
}

void startWrite(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    while (user->isWriting || user->readCount > 0) {
        pthread_cond_wait(&user->cond, &user->mutex);
    }
    user->isWriting = 1;
    pthread_mutex_unlock(&user->mutex);
}

void finishWrite(UserInfo *user) {
    pthread_mutex_lock(&user->mutex);
    user->isWriting = 0;
    pthread_cond_broadcast(&user->cond);
    pthread_mutex_unlock(&user->mutex);
}
*/