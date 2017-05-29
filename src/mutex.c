#include "mutex.h"

#include "common.h"

void mutex_init(stMutex_t* mutex) {
  pthread_mutex_init(&mutex->mutex, NULL);
}
void mutex_lock(stMutex_t* mutex) {
  pthread_mutex_lock(&mutex->mutex);
}
void mutex_unlock(stMutex_t *mutex) {
  pthread_mutex_unlock(&mutex->mutex);
}
void mutex_destroy(stMutex_t *mutex) {
  pthread_mutex_destroy(&mutex->mutex);
}
