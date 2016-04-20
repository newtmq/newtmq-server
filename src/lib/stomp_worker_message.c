#include <kazusa/common.h>
#include <kazusa/logger.h>
#include <kazusa/queue.h>
#include <kazusa/stomp.h>
#include <kazusa/stomp_message_worker.h>

/* This list head is used by stomp_message_worker */
typedef struct msg_management_t {
  pthread_mutex_t mutex;
  struct list_head h_msg;
} msg_management_t;
static msg_management_t msgm;

void stomp_message_worker_init() {
  INIT_LIST_HEAD(&msgm.h_msg);
  pthread_mutex_init(&msgm.mutex, NULL);
}

stomp_msginfo_t *alloc_msginfo() {
  stomp_msginfo_t *ret;

  ret = (stomp_msginfo_t *)malloc(sizeof(stomp_msginfo_t));
  if(ret == NULL) {
    logger(LOG_ERROR, "[alloc_msginfo] failed to allocate memory");
    return NULL;
  }

  /* Initialize object */
  memset(ret, 0, sizeof(stomp_msginfo_t));
  INIT_LIST_HEAD(&ret->list);

  return ret;
}

void free_msginfo(stomp_msginfo_t *msginfo) {
  if(msginfo != NULL) {
    free(msginfo);
  }
}

int stomp_message_register(stomp_msginfo_t *info) {
  pthread_mutex_lock(&msgm.mutex);
  {
    logger(LOG_DEBUG, "(stomp_message_register) &msgm.h_msg: %p", &msgm.h_msg);

    list_add(&info->list, &msgm.h_msg);
  }
  pthread_mutex_unlock(&msgm.mutex);
}

static int do_send_message_frames(stomp_msginfo_t *msginfo) {
  frame_t *frame;
  int index = 0;
  char hdr_dest[LD_MAX];
  char hdr_msgid[LD_MAX];
  linedata_t *body;

  sprintf(hdr_dest, "destination: %s\n", msginfo->qname);

  while((frame = (frame_t *)dequeue(msginfo->qname)) != NULL) {
    /* making message-id attribute for each message */
    sprintf(hdr_msgid, "message-id: %s-%d\n", msginfo->qname, index++);

    send_msg(msginfo->sock, "MESSAGE\n");
    send_msg(msginfo->sock, hdr_dest);
    send_msg(msginfo->sock, hdr_msgid);
    send_msg(msginfo->sock, "\n");
    list_for_each_entry(body, &frame->h_data, l_frame) {
      send_msg(msginfo->sock, body->data);
    }
    send_msg(msginfo->sock, NULL);
  }
}

void *stomp_message_worker(void *data) {
  logger(LOG_DEBUG, "[stomp_message_worker] started (&msgm.h_msg: %p)", &msgm.h_msg);

  while(1) {
    stomp_msginfo_t *msginfo;
    frame_t *frame;

    if(! list_empty(&msgm.h_msg)) {
      pthread_mutex_lock(&msgm.mutex);
      {
        msginfo = list_first_entry(&msgm.h_msg, stomp_msginfo_t, list);
        list_del(&msginfo->list);
      }
      pthread_mutex_unlock(&msgm.mutex);

      do_send_message_frames(msginfo);

      free_msginfo(msginfo);
    } else {
      pthread_yield();
    }
  }
  logger(LOG_DEBUG, "[stomp_message_worker] finished");
}
