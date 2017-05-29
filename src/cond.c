#include "cond.h"

void cond_init(stCond_t* cond) {
  cond->cnt = 0;
  pthread_cond_init(&cond->cond, NULL);
  pthread_mutex_init(&cond->mutex, NULL);
}

void cond_wait(stCond_t* cond) {
  pthread_mutex_lock(&cond->mutex);
  while (cond->cnt == 0) {
    pthread_cond_wait(&cond->cond, &cond->mutex);
  }
  if (cond->cnt > 0) {
    cond->cnt--;
  }
  pthread_mutex_unlock(&cond->mutex);
}

void cond_wake(stCond_t *cond) {
  pthread_mutex_lock(&cond->mutex);
  cond->cnt++;
  if (cond->cnt > 0) {
    pthread_cond_signal(&cond->cond);
  }
  pthread_mutex_unlock(&cond->mutex);
}

void cond_destroy(stCond_t *cond) {
  pthread_cond_destroy(&cond->cond);
  pthread_mutex_destroy(&cond->mutex);
}
