#include <newt/list.h>
#include <newt/frame.h>
#include <newt/common.h>
#include <newt/config.h>
#include <newt/persistent_worker.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define FILEPATH_MAX 512

enum worker_status {
  STATUS_STOP = 1 << 0,
};

// This data-structure is corresponding to queue
typedef struct queue_info {
  unsigned long qid; // hashed id generated from qname by queue processing
  FILE *fp_context;
  FILE *fp_index_sent;

  struct list_head l_pm; // linked with persistent_manager
  struct list_head h_unpersistent; // linked with frame_t
  pthread_mutex_t m_unpersistent;

  // This means the whole bytes of queue (updated by 'persistent' function)
  long index_head; 

  // This means the bytes which is already sent to subscriber in queue (updated by 'sent_frame' function)
  long index_sent; 

  // This specifes the index which is already persistent (updated in flush processing)
  long index_persistent; 
} queue_info_t;

// This is make a single object which is 'pm'
typedef struct persistent_manager {
  pthread_t worker_id;
  struct list_head h_qinfo;
  pthread_mutex_t m_qinfo;
  char *datadir;
  int status;
} persistent_manager_t;

static persistent_manager_t pm;

static queue_info_t *get_qinfo(unsigned long id) {
  queue_info_t *qinfo, *ret = NULL;

  list_for_each_entry(qinfo, &pm.h_qinfo, l_pm) {
    if(qinfo->qid == id) {
      ret = qinfo;
    }
  }

  return ret;
}

static queue_info_t *alloc_qinfo(char *qname) {
  char dirpath[FILEPATH_MAX] = {0};
  char filepath[FILEPATH_MAX] = {0};
  queue_info_t *qinfo = NULL;

  qinfo = (queue_info_t *)malloc(sizeof(queue_info_t));
  if(qinfo != NULL) {
    struct stat st = {0};
    sprintf(dirpath, "%s/%s", pm.datadir, qname);

    // make queue directory if it doesn't exist
    if(stat(dirpath, &st) < 0) {
      mkdir_recursive(dirpath);
    } else if(! S_ISDIR(st.st_mode)) {
      warn("[alloc_qinfo] queue directory (%s) is not directory", dirpath);
      free(qinfo);

      return NULL;
    }

    qinfo->qid = 0L;
    qinfo->index_head = 0;
    qinfo->index_sent = 0;
    qinfo->index_persistent = 0;

    sprintf(filepath, "%s/data", dirpath);
    qinfo->fp_context = fopen(filepath, "ab");
    if(qinfo->fp_context == NULL) {
      warn("[alloc_qinfo] failed to open queue data file (%s) [0x%x]", filepath, errno);
      free(qinfo);
      return NULL;
    }
    sprintf(filepath, "%s/index_sent", dirpath);
    qinfo->fp_index_sent = fopen(filepath, "wb");
    if(qinfo->fp_index_sent == NULL) {
      warn("[alloc_qinfo] failed to open metadata file of sent-index (%s) [0x%x]", filepath, errno);
      free(qinfo);
      return NULL;
    }

    INIT_LIST_HEAD(&qinfo->l_pm);
    INIT_LIST_HEAD(&qinfo->h_unpersistent);

    pthread_mutex_init(&qinfo->m_unpersistent, NULL);
  }

  return qinfo;
}

static void free_qinfo(queue_info_t *qinfo) {
  frame_t *frame, *f;

  pthread_mutex_lock(&qinfo->m_unpersistent);
  list_for_each_entry_safe(frame, f, &qinfo->h_unpersistent, l_persistent) {
    list_del(&frame->l_persistent);
  }
  pthread_mutex_unlock(&qinfo->m_unpersistent);

  fclose(qinfo->fp_context);
  fclose(qinfo->fp_index_sent);
  free(qinfo);
}

static int add_frame_into_qinfo(queue_info_t *qinfo, frame_t *frame) {
  pthread_mutex_lock(&qinfo->m_unpersistent);
  {
    list_add_tail(&frame->l_persistent, &qinfo->h_unpersistent);
  }
  pthread_mutex_unlock(&qinfo->m_unpersistent);

  return RET_SUCCESS;
}

static void flush_queue_info(queue_info_t *qinfo) {
  fseek(qinfo->fp_index_sent, 0, SEEK_SET);
  fwrite((const void *)&qinfo->index_sent, sizeof(long), 1, qinfo->fp_index_sent);
  fflush(qinfo->fp_index_sent);
}

static int flush_queue_context(queue_info_t *qinfo) {
  frame_t *frame;

  pthread_mutex_lock(&qinfo->m_unpersistent);
  list_for_each_entry(frame, &qinfo->h_unpersistent, l_persistent) {
    linedata_t *header, *body;

    // write out frame name
    fwrite(frame->name, strlen(frame->name), 1, qinfo->fp_context);
    fwrite("\n", sizeof(char), 1, qinfo->fp_context); // separation

    // write out headers
    pthread_mutex_lock(&frame->mutex_header);
    list_for_each_entry(header, &frame->h_attrs, list) {
      fwrite(header->data, header->len, 1, qinfo->fp_context);
      fwrite("\n", sizeof(char), 1, qinfo->fp_context); // separation
    }
    pthread_mutex_unlock(&frame->mutex_header);
    fwrite("\n", sizeof(char), 1, qinfo->fp_context); // separation
    
    // write out body
    pthread_mutex_lock(&frame->mutex_body);
    list_for_each_entry(body, &frame->h_data, list) {
      fwrite(body->data, body->len, 1, qinfo->fp_context);
    }
    pthread_mutex_unlock(&frame->mutex_body);
    fwrite("\0", sizeof(char), 1, qinfo->fp_context); // separation

    fflush(qinfo->fp_context);

    qinfo->index_persistent += frame->size;
  }
  pthread_mutex_unlock(&qinfo->m_unpersistent);

  return RET_SUCCESS;
}

static void *persistent_worker(void *_arg) {
  while(! GET((&pm), STATUS_STOP)) {
    queue_info_t *qinfo;

    if(! list_empty(&pm.h_qinfo)) list_for_each_entry(qinfo, &pm.h_qinfo, l_pm) {
      if(qinfo->index_head > qinfo->index_persistent) {
        flush_queue_info(qinfo);
        flush_queue_context(qinfo);
      }
    }
    pthread_yield();
  }

  return NULL;
}

static void stop_persistent_worker() {
  SET((&pm), STATUS_STOP);
}

static void cleanup_persistent_list() {
  queue_info_t *qinfo, *q;

  pthread_mutex_lock(&pm.m_qinfo);
  list_for_each_entry_safe(qinfo, q, &pm.h_qinfo, l_pm) {
    list_del(&qinfo->l_pm);
    free_qinfo(qinfo);
  }
  pthread_mutex_unlock(&pm.m_qinfo);
}

int initialize_persistent_worker(newt_config *conf) {
  int ret = RET_ERROR;

  if(conf->datadir != NULL) {
    INIT_LIST_HEAD(&pm.h_qinfo);
    pthread_mutex_init(&pm.m_qinfo, NULL);
  
    pm.datadir = conf->datadir;
    pm.status = 0;
  
    struct stat st = {0};
    if (stat(conf->datadir, &st) < 0) {
      mkdir(conf->datadir, 0755);
    } else if(! S_ISDIR(st.st_mode)) {
      warn("[initialize_persistent_worker] failed to open datadir (%s)", conf->datadir);
      return RET_ERROR;
    }
  
    set_signal_handler(cleanup_persistent_worker, NULL);
    pthread_create(&pm.worker_id, NULL, persistent_worker, NULL);

    ret = RET_SUCCESS;
  }

  return ret;
}

int cleanup_persistent_worker() {
  stop_persistent_worker();
  cleanup_persistent_list();

  return RET_SUCCESS;
}

int persistent(const char *qname, frame_t *frame) {
  queue_info_t *qinfo;

  assert(frame != NULL);

  qinfo = get_qinfo(get_hash((unsigned char *)qname));
  if(qinfo == NULL) {
    qinfo = alloc_qinfo((char *)qname);

    assert(qinfo != NULL);
    
    pthread_mutex_lock(&pm.m_qinfo);
    {
      list_add_tail(&qinfo->l_pm, &pm.h_qinfo);
    }
    pthread_mutex_unlock(&pm.m_qinfo);
  }

  add_frame_into_qinfo(qinfo, frame);
  qinfo->index_head += frame->size;

  pthread_yield();

  return RET_SUCCESS;
}

int update_index_sent(const char *qname, frame_t *frame) {
  queue_info_t *qinfo;
  int ret = RET_ERROR;

  assert(frame != NULL);

  qinfo = get_qinfo(get_hash((unsigned char *)qname));
  if(qinfo != NULL) {
    qinfo->index_sent += frame->size;

    ret = RET_SUCCESS;
  }

  return ret;
}
