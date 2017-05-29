#ifndef _LIST_H_
#define _LIST_H_

#include "common.h"

typedef struct _stListItem {
  struct _stListItem *prev;
  struct _stListItem *next;
  void *data;
}stListItem_t;

typedef struct _stList {
  stListItem_t *head;
  stListItem_t *tail;
  int size;
}stList_t;

void list_init(stList_t *l);
bool list_push_front(stList_t *l, void *data);
bool list_push_back(stList_t *l, void *data);
bool list_pop_front(stList_t *l, void **data);
bool list_pop_back(stList_t *l, void **data);
bool list_peek_front(stList_t *l, void **data);
bool list_peek_back(stList_t *l, void **data);
void list_destroy(stList_t *l, void (*freefunc)(void*));
int  list_size(stList_t *l);
bool list_null();

#endif
