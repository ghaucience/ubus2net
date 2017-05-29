#ifndef _LOCK_QUEUE_H_
#define _LOCK_QUEUE_H_

#include "mutex.h"
#include "cond.h"
#include "list.h"

typedef struct _stLockCondQueue {
  stMutex_t mtx;
  stCond_t cond;
  stList_t list;
}stLockQueue_t;

void lockqueue_init(stLockQueue_t *lq);
void lockqueue_push(stLockQueue_t *lq, void *elem);
bool lockqueue_pop(stLockQueue_t *lq, void **elem);
bool lockqueue_pop_back(stLockQueue_t *lq, void **elem);
void lockqueue_destroy(stLockQueue_t *lq, void (*free_elem)(void*));
void lockqueue_wake(stLockQueue_t *lq);
void lockqueue_wait(stLockQueue_t *lq);
int  lockqueue_size(stLockQueue_t *lq);
bool lockqueue_empty(stLockQueue_t *lq);

#endif
