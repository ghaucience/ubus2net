#include "lockqueue.h"
#include "common.h"


void lockqueue_init(stLockQueue_t *lq) {
  mutex_init(&lq->mtx);
  cond_init(&lq->cond);
  list_init(&lq->list);
}
void lockqueue_push(stLockQueue_t *lq, void *elem) {
  mutex_lock(&lq->mtx);
	list_push_front(&lq->list, elem);
  mutex_unlock(&lq->mtx);
}
bool lockqueue_pop(stLockQueue_t *lq, void **elem) {
  bool ret = false;
  mutex_lock(&lq->mtx);
	if (!list_empty(&lq->list)) {
    list_pop_back(&lq->list, elem);
    ret = true;
  }
  mutex_unlock(&lq->mtx);
  return ret;
}
bool lockqueue_pop_back(stLockQueue_t *lq, void **elem) {
  bool ret = false;
  mutex_lock(&lq->mtx);
	if (!list_empty(&lq->list)) {
    list_pop_front(&lq->list, elem);
    ret = true;
  }
  mutex_unlock(&lq->mtx);
  return ret;
}
void lockqueue_destroy(stLockQueue_t *lq, void (*free_elem)(void*)) {
  mutex_lock(&lq->mtx);
  list_destroy(&lq->list, free_elem);
  mutex_unlock(&lq->mtx);
  
  mutex_destroy(&lq->mtx);
  cond_destroy(&lq->cond);
}

void lockqueue_wake(stLockQueue_t *lq) {
	cond_wake(&lq->cond);
}

void lockqueue_wait(stLockQueue_t *lq) {
	cond_wait(&lq->cond);
}

int    lockqueue_size(stLockQueue_t *lq) {
  return list_size(&lq->list);
}

bool lockqueue_empty(stLockQueue_t *lq) {
	return (lockqueue_size(lq) == 0);
}
