#include <newt/transaction.h>
#include <newt/common.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct transaction_manager {
  struct list_head list;
  pthread_mutex_t mutex;
};
static struct transaction_manager manager;

static transaction_t *get_transaction(char *tid) {
  transaction_t *ret = NULL;

  pthread_mutex_lock(&manager.mutex);
  {
    transaction_t *t;
    list_for_each_entry(t, &manager.list, l_manager) {
      if(strncmp(t->tid, tid, strlen(tid)) == 0) {
        ret = t;
      }
    }
  }
  pthread_mutex_unlock(&manager.mutex);

  return ret;
}

static transaction_t *alloc_transaction(char *tid) {
  transaction_t *obj;

  obj = (transaction_t *)malloc(sizeof(transaction_t));
  if(obj != NULL) {
    memcpy(obj->tid, tid, strlen(tid));

    INIT_LIST_HEAD(&obj->l_manager);
    INIT_LIST_HEAD(&obj->h_frame);
    pthread_mutex_init(&obj->mutex, NULL);
  }

  pthread_mutex_lock(&manager.mutex);
  {
    list_add_tail(&obj->l_manager, &manager.list);
  }
  pthread_mutex_unlock(&manager.mutex);

  return obj;
}

static int free_transaction(transaction_t *obj) {
  int ret = RET_ERROR;

  if(obj != NULL) {
    pthread_mutex_lock(&manager.mutex);
    {
      list_del(&obj->l_manager);
    }
    pthread_mutex_unlock(&manager.mutex);

    free(obj);

    ret = RET_SUCCESS;
  }

  return ret;
}

int transaction_init() {
  INIT_LIST_HEAD(&manager.list);
  pthread_mutex_init(&manager.mutex, NULL);

  return RET_SUCCESS;
}

int transaction_destruct() {
  transaction_t *obj, *t;

  pthread_mutex_lock(&manager.mutex);
  {
    list_for_each_entry_safe(obj, t, &manager.list, l_manager) {
      free_transaction(obj);
    }
  }
  pthread_mutex_unlock(&manager.mutex);

  return RET_SUCCESS;
}

int transaction_start(char *tid) {
  transaction_t *obj;
  int ret = RET_ERROR;

  obj = get_transaction(tid);
  if(obj == NULL) {
    obj = alloc_transaction(tid);
    ret = RET_SUCCESS;
  }

  return ret;
}

int transaction_add(char *tid, frame_t *frame) {
  transaction_t *obj;
  int ret = RET_ERROR;

  assert(tid != NULL);

  obj = get_transaction(tid);
  if(obj != NULL) {
    pthread_mutex_lock(&obj->mutex);
    {
      list_add(&frame->l_transaction, &obj->h_frame);
    }
    pthread_mutex_unlock(&obj->mutex);

    ret = RET_SUCCESS;
  }

  return ret;
}

int transaction_abort(char *tid) {
  transaction_t *obj;
  int ret = RET_ERROR;

  assert(tid != NULL);

  obj = get_transaction(tid);
  if(obj != NULL) {
    free_transaction(obj);

    ret = RET_SUCCESS;
  }

  return ret;
}

int transaction_commit(char *tid) {
  transaction_t *obj;
  int ret = RET_ERROR;

  assert(tid != NULL);

  obj = get_transaction(tid);
  if(obj != NULL) {
    int ret_callback = RET_SUCCESS;

    pthread_mutex_lock(&obj->mutex);
    while(! list_empty(&obj->h_frame)) {
      frame_t *frame = list_first_entry(&obj->h_frame, frame_t, l_transaction);
      if(frame != NULL && frame->transaction_callback != NULL) {
        int ret_callback = frame->transaction_callback(frame);

        list_del(&frame->l_transaction);

        if(ret_callback == RET_ERROR) {
          warn("[transaction_commit] failed to handle frame (%s)", frame->name);
          break;
        }
      }
    }
    pthread_mutex_unlock(&obj->mutex);

    free_transaction(obj);

    if(ret_callback == RET_SUCCESS) {
      ret = RET_SUCCESS;
    }
  }

  return ret;
}
