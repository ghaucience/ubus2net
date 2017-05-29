#ifndef _COND_H_
#define _COND_H_

#include <pthread.h>

typedef struct _stCond {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int cnt;
}stCond_t;

void cond_init(stCond_t* cond);
void cond_wait(stCond_t* cond);
void cond_wake(stCond_t *cond);
void cond_destroy(stCond_t *cond);


#endif
