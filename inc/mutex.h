#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <pthread.h>

typedef struct _stMutex {
  pthread_mutex_t mutex;
}stMutex_t;

void mutex_init(stMutex_t* mutex);
void mutex_lock(stMutex_t* mutex);
void mutex_unlock(stMutex_t *mutex);
void mutex_destroy(stMutex_t *mutex);

#endif
