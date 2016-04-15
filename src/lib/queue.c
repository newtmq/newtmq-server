#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <kazusa/common.h>
#include <kazusa/list.h>
#include <kazusa/queue.h>

struct list_head *queuebox[QB_SIZE] = {0};
static pthread_mutex_t queuebox_lock;

static unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;
  
  while (c = *str++) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  
  return hash;
}

static struct q_entry *create_entry(void *data) {
  struct q_entry *e;

  e = (struct q_entry*)malloc(sizeof(struct q_entry));
  if(e != NULL) {
    e->data = data;
    INIT_LIST_HEAD(&e->l_queue);
  }

  return e;
}

static struct queue *create_queue() {
  struct queue *q;
  
  q = (struct queue*)malloc(sizeof(struct queue));
  if(q != NULL) {
    q->hashnum = 0;
    pthread_mutex_init(&q->mutex, NULL);
    INIT_LIST_HEAD(&q->l_box);
    INIT_LIST_HEAD(&q->h_entry);
  }

  return q;
}

static void clear_entry(struct q_entry *e) {
  if(e != NULL) {
    free(e);
  }
}

static void clear_queue(struct queue* q) {
  struct q_entry *e;

  if(q != NULL) {
    list_for_each_entry(e, &q->h_entry, l_queue) {
      clear_entry(e);
    }
  }
}

static struct queue *get_queue(char *qname) {
  unsigned long hashnum = hash(qname);
  int index = (int) hashnum % QB_SIZE;
  struct queue *queue = NULL;

  /* create new_queue*/
  pthread_mutex_lock(&queuebox_lock);
  if(queuebox[index] == NULL) {
    struct list_head *head = (struct list_head *)malloc(sizeof(struct list_head));
    if(head == NULL) {
      return NULL;
    }
    INIT_LIST_HEAD(head);

    queuebox[index] = head;
  }
  pthread_mutex_unlock(&queuebox_lock);

  /* get queue */
  struct queue *q;
  pthread_mutex_lock(&queuebox_lock);
  list_for_each_entry(q, queuebox[index], l_box) {
    if(q->hashnum == hashnum) {
      queue = q;
    }
  }
  pthread_mutex_unlock(&queuebox_lock);

  if(queue == NULL) {
    /* target queue doesn't exist yet */
    queue = create_queue();
    if(queue != NULL) {
      queue->hashnum = hashnum;
    }
  }

  return queue;
}

int enqueue(void *data, char *qname) {
  struct q_entry* e;
  struct queue* q;

  if(data == NULL) {
    return RET_ERROR;
  }

  e = create_entry(data);
  if(e == NULL) {
    return RET_ERROR;
  }

  q = get_queue(qname);
  if(q == NULL) {
    clear_entry(e);
    return RET_ERROR;
  }

  /* set entry to queue */
  pthread_mutex_lock(&q->mutex);
  {
    list_add_tail(&e->l_queue, &q->h_entry);
  }
  pthread_mutex_unlock(&q->mutex);

  return RET_SUCCESS;
}

void *dequeue(char *qname) {
  struct q_entry *e = NULL;
  struct queue *q;
  void *ret = NULL;

  q = get_queue(qname);
  if(q == NULL) {
    return RET_ERROR;
  }

  pthread_mutex_lock(&q->mutex);
  {
    if(! list_empty(&q->h_entry)) {
      e = list_first_entry(&q->h_entry, struct q_entry, l_queue);
      list_del(&e->l_queue);

      ret = e->data;
    }
  }
  pthread_mutex_unlock(&q->mutex);

  return e;
}

int cleanup_queuebox() {
  int i;

  pthread_mutex_lock(&queuebox_lock);
  {
    for(i=0; i<QB_SIZE; i++) {
      clear_queue(queuebox[i]);
    }
  }
  pthread_mutex_unlock(&queuebox_lock);

  return RET_SUCCESS;
}

int initialize_queuebox() {
  memset(queuebox, 0, sizeof(queuebox));
  pthread_mutex_init(&queuebox_lock, NULL);

  return RET_SUCCESS;
}
