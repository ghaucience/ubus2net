#include "list.h"

void list_init(stList_t *l) {
  ASSERT(l != NULL);

  l->head = l->tail = NULL;
  l->size = 0;
}

bool list_push_front(stList_t *l, void *data) {
  ASSERT(l != NULL);
  
  stListItem_t *pi = (stListItem_t *)MALLOC(sizeof(stListItem_t));
  ASSERT(pi != NULL);
  
  pi->data = data;
  pi->next = NULL;
  pi->prev = NULL;

  pi->next = l->head;
  if (l->head != NULL) {
    l->head->prev = pi;
  }
  
  l->head = pi;
  if (l->tail == NULL) {
    l->tail = pi;
  }
  l->size++;
	return true;
}

bool list_push_back(stList_t *l, void *data) {
  ASSERT(l != NULL);
  
  stListItem_t *pi = (stListItem_t *)MALLOC(sizeof(stListItem_t));
  ASSERT(pi != NULL);
  
  pi->data = data;
  pi->next = NULL;
  pi->prev = NULL;

  if (l->tail != NULL) {
    l->tail->next = pi;
    pi->prev = l->tail;
  }
  
  if (l->head == NULL)  {
    l->head = pi;
  }
  l->tail = pi;
  l->size++;
	return true;
}

bool list_pop_front(stList_t *l, void **data) {
  bool ret = false;
  
  ASSERT(l != NULL);
  ASSERT(data != NULL);
	*data = NULL;

  if (l->size > 0) {
    ASSERT(l->head != NULL);
    stListItem_t *pi = l->head;
    //pi->next = NULL;

    l->head = l->head->next;
    if (l->head != NULL) {
      l->head->prev = NULL;
    }
    l->size--;

    if (l->size == 0) {
      l->tail = NULL;
    }

    *data = pi->data;
    
    FREE(pi);
    
    ret = true;
  }

  return ret;
}

bool list_pop_back(stList_t *l, void **data) {
  bool ret = false;
  
  ASSERT(l != NULL);
  ASSERT(data != NULL);
	*data = NULL;

  if (l->size > 0) {
    ASSERT(l->tail != NULL);
    stListItem_t *pi = l->tail;
    //pi->prev = NULL;

    l->tail = l->tail->prev;
    if (l->tail != NULL) {
      l->tail->next = NULL;
    }
    l->size--;

    if (l->size == 0) {
      l->head = NULL;
    }

    *data = pi->data;
    
    FREE(pi);
    
    ret = true;
  }

  return ret;  

}


bool list_peek_front(stList_t *l, void **data) {
  bool ret = false;
  
  ASSERT(l != NULL);
  ASSERT(data != NULL);
	*data = NULL;

  if (l->size > 0) {
    ASSERT(l->head != NULL);
    stListItem_t *pi = l->head;

    *data = pi->data;

    ret = true;
  }

  return ret;

}
bool list_peek_back(stList_t *l, void **data) {
  bool ret = false;
  
  ASSERT(l != NULL);
  ASSERT(data != NULL);

  if (l->size > 0) {
    ASSERT(l->tail != NULL);
    stListItem_t *pi = l->tail;

    *data = pi->data;
    
    ret = true;
  }

  return ret;  
}

void list_destroy(stList_t *l, void (*freefunc)(void *)) {
  ASSERT(l != NULL);
  
  stListItem_t *pi = (stListItem_t *)MALLOC(sizeof(stListItem_t));
  pi = l->head;
  
  while (pi != NULL) {
    l->head = pi->next;

		if (pi->data != NULL) {
			if (freefunc != NULL) {
				freefunc(pi->data);
				pi->data = NULL;
			}
		}

    FREE(pi);
    l->size--;
    
    pi = l->head->next;
  }
  l->tail = l->head;

  ASSERT(l->size == 0);
  ASSERT(l->tail == NULL);
}

int    list_size(stList_t *l) {
  ASSERT(l != NULL);
  return l->size;
}

bool list_empty(stList_t *l) {
  ASSERT(l != NULL);
	return (list_size(l) == 0);
}
