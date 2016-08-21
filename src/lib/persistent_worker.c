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
#include <dirent.h>
#include <errno.h>

#define FNAME_DATA "data"
#define FNAME_SENT_INDEX "sent_index"

#define MAX_PATH_LENGTH 512
#define BUFFER_LEN 1024

enum worker_status {
  STATUS_INITIALIZED = 1 << 0,
  STATUS_STOP        = 2 << 0,
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
  struct list_head h_qinfo;
  pthread_mutex_t m_qinfo;
  char *datadir;
  int status;
} persistent_manager_t;

static persistent_manager_t pm = {0};

static queue_info_t *get_qinfo(unsigned long id) {
  queue_info_t *qinfo, *ret = NULL;

  list_for_each_entry(qinfo, &pm.h_qinfo, l_pm) {
    if(qinfo->qid == id) {
      ret = qinfo;
    }
  }

  return ret;
}

static void persist_queue_info(queue_info_t *qinfo) {
  fseek(qinfo->fp_index_sent, 0, SEEK_SET);
  fwrite((const void *)&qinfo->index_sent, sizeof(long), 1, qinfo->fp_index_sent);
  fflush(qinfo->fp_index_sent);
}

static long unpersist_queue_info(char *dirpath) {
  char filepath[MAX_PATH_LENGTH];
  long ret = 0;
  FILE *fp;

  sprintf(filepath, "%s/%s", dirpath, FNAME_SENT_INDEX);
  fp = fopen(filepath, "rb");
  if(fp == NULL || fread((void *)&ret, sizeof(long), 1, fp) == 0 || ferror(fp) > 0) {
    warn("[unpersist_queue_info] failed to load from file [errno: 0x%x]", errno);
  }
  fclose(fp);

  return ret;
}

static queue_info_t *alloc_qinfo(char *qname) {
  char dirpath[MAX_PATH_LENGTH] = {0};
  char filepath[MAX_PATH_LENGTH] = {0};
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

    qinfo->qid = get_hash(qname);
    qinfo->index_head = 0;
    qinfo->index_sent = 0;
    qinfo->index_persistent = 0;

    sprintf(filepath, "%s/%s", dirpath, FNAME_DATA);
    qinfo->fp_context = fopen(filepath, "ab");
    if(qinfo->fp_context == NULL) {
      warn("[alloc_qinfo] failed to open queue data file (%s) [0x%x]", filepath, errno);
      free(qinfo);
      return NULL;
    }
    sprintf(filepath, "%s/%s", dirpath, FNAME_SENT_INDEX);

    // reinstate metadata
    if(stat(filepath, &st) == 0) {
      qinfo->index_sent = unpersist_queue_info(dirpath);
      qinfo->index_head = st.st_size;
      qinfo->index_persistent = st.st_size;
    }

    qinfo->fp_index_sent = fopen(filepath, "wb");
    if(qinfo->fp_index_sent == NULL) {
      warn("[alloc_qinfo] failed to open metadata file of sent-index (%s) [0x%x]", filepath, errno);
      free(qinfo);
      return NULL;
    }
    persist_queue_info(qinfo);

    INIT_LIST_HEAD(&qinfo->l_pm);
    INIT_LIST_HEAD(&qinfo->h_unpersistent);

    pthread_mutex_init(&qinfo->m_unpersistent, NULL);
  }

  return qinfo;
}

// This method get queue_info_t object from persistent_manager_t (PM) object.
// When it fails which means target queue_info_t doens't exist yet, this function allocate it
// and links to PM object list_head.
static queue_info_t *create_qinfo(char *qname) {
  queue_info_t *qinfo;

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

static struct frame_info *alloc_frame_info() {
  struct frame_info *finfo;

  finfo = (struct frame_info *)malloc(sizeof(struct frame_info));
  if(finfo != NULL) {
    finfo->frame = NULL;
    INIT_LIST_HEAD(&finfo->list);
  }

  return finfo;
}

static void free_frame_info(struct frame_info *finfo) {
  list_del(&finfo->list);
  free(finfo);
}

static int add_frame_into_qinfo(queue_info_t *qinfo, frame_t *frame) {
  pthread_mutex_lock(&qinfo->m_unpersistent);
  {
    list_add_tail(&frame->l_persistent, &qinfo->h_unpersistent);
  }
  pthread_mutex_unlock(&qinfo->m_unpersistent);

  return RET_SUCCESS;
}

static int unpersist_queue_context(char *dirpath, struct list_head *head) { // XXX: nocompletion
  char filepath[MAX_PATH_LENGTH];
  char frame_buffer[BUFFER_LEN];
  long sent_index = 0;
  FILE *fp;

  assert(head != NULL);

  // read sent index
  sent_index = unpersist_queue_info(dirpath);
  if(sent_index < 0) {
    return RET_ERROR;
  }

  // read frames
  sprintf(filepath, "%s/%s", dirpath, FNAME_DATA);
  fp = fopen(filepath, "r");
  if(fp != NULL) {
    ssize_t len;
    frame_t *frame = NULL;

    fseek(fp, sent_index, SEEK_SET);

    while((len = fread(frame_buffer, sizeof(char), BUFFER_LEN, fp)) > 0) {
      int offset, index = 0;

      do {
        if(frame == NULL) {
          frame = alloc_frame();
        }

        if(parse_frame(frame, frame_buffer + index, (int)len, &offset) == RET_SUCCESS) {
          struct frame_info *finfo;

          finfo = alloc_frame_info();
          if(finfo != NULL) {
            finfo->frame = frame;
            list_add_tail(&finfo->list, head);

            frame = NULL;
          }
        }

        index += offset;
      } while(index < len);
    }

    if(ferror(fp) > 0) {
      warn("[do_load_persistent_queue] failed to read persistent_queue %s [0x%x]", filepath, errno);
    }
  }
  fclose(fp);

  return RET_SUCCESS;
}

static int persist_queue_context(queue_info_t *qinfo) {
  frame_t *frame, *f;

  pthread_mutex_lock(&qinfo->m_unpersistent);
  list_for_each_entry_safe(frame, f, &qinfo->h_unpersistent, l_persistent) {
    list_del(&frame->l_persistent);

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

void *persistent_worker(void *_arg) {
  // waiting until persistent_manager object is initialized
  if(! GET((&pm), STATUS_INITIALIZED)) {
    pthread_yield();
  }

  while(! GET((&pm), STATUS_STOP)) {
    queue_info_t *qinfo;

    pthread_mutex_lock(&pm.m_qinfo);
    if(! list_empty(&pm.h_qinfo)) list_for_each_entry(qinfo, &pm.h_qinfo, l_pm) {
      persist_queue_info(qinfo);

      if(qinfo->index_head > qinfo->index_persistent) {
        persist_queue_context(qinfo);
      }
    }
    pthread_mutex_unlock(&pm.m_qinfo);

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

static int cleanup_persistent_worker() {
  stop_persistent_worker();
  cleanup_persistent_list();

  return RET_SUCCESS;
}

int initialize_persistent_worker(newt_config *conf) {
  int ret = RET_ERROR;

  if(conf->datadir != NULL) {
    INIT_LIST_HEAD(&pm.h_qinfo);
    pthread_mutex_init(&pm.m_qinfo, NULL);
  
    pm.datadir = conf->datadir;
    pm.status = STATUS_INITIALIZED;
  
    struct stat st = {0};
    if (stat(conf->datadir, &st) < 0) {
      mkdir(conf->datadir, 0755);
    } else if(! S_ISDIR(st.st_mode)) {
      warn("[initialize_persistent_worker] failed to open datadir (%s)", conf->datadir);
      return RET_ERROR;
    }

    set_signal_handler(cleanup_persistent_worker, NULL);

    ret = RET_SUCCESS;
  }

  return ret;
}

int unpersist() {
  char dirpath[MAX_PATH_LENGTH] = {0};
  char datapath[MAX_PATH_LENGTH] = {0};
  struct dirent *dir;
  DIR *d;

  if(! GET((&pm), STATUS_INITIALIZED)) {
    return RET_ERROR;
  }

  d = opendir(pm.datadir);
  if(d) while((dir = readdir(d)) != NULL) {
    struct list_head frame_head;
    queue_info_t *qinfo;
    struct stat st = {0};

    if(dir->d_name[0] == '.') {
      continue;
    }

    INIT_LIST_HEAD(&frame_head);

    sprintf(dirpath, "%s/%s", pm.datadir, dir->d_name);

    // create queue_info object
    qinfo = create_qinfo(dir->d_name);
    qinfo->index_sent = unpersist_queue_info(dirpath);

    // get metadata of target queue and initialize the object of queue_info_t
    sprintf(datapath, "%s/%s", dirpath, FNAME_DATA);
    if(stat(datapath, &st) == 0 && S_ISDIR(st.st_mode)) {
      warn("[unpersist] failed to get stat from '%s'", datapath);
      continue;
    }
    qinfo->index_head = qinfo->index_persistent= st.st_size;

    // get frames and reinstate it
    if(unpersist_queue_context(dirpath, &frame_head) == RET_ERROR) {
      warn("[unpersist] failed to unpersist frames in '%s'", dirpath);
      free_qinfo(qinfo);
      continue;
    }

    if(! list_empty(&frame_head)) {
      struct frame_info *finfo, *f;

      list_for_each_entry_safe(finfo, f, &frame_head, list) {
        // because directory name shows the queue name
        enqueue((void *)finfo->frame, dir->d_name);

        // cleanup processing
        free_frame_info(finfo);
      }
    }
  }

  return RET_SUCCESS;
}

int persist_frame(frame_t *frame, char *qname) {
  queue_info_t *qinfo;

  assert(frame != NULL);

  qinfo = create_qinfo(qname);

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

    pthread_yield();

    ret = RET_SUCCESS;
  }

  return ret;
}
