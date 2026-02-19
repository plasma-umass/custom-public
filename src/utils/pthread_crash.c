#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

int pthread_create(pthread_t* restrict thread,
                   const pthread_attr_t* restrict attr,
                   void* (*start_routine)(void*), void* restrict arg) {
    (void) thread;
    (void) attr;
    (void) start_routine;
    (void) arg;
    fprintf(stderr, "[ERROR] The program tried calling pthread_create...\n");
    exit(EXIT_FAILURE);
}
