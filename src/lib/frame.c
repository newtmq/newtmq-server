#include <newt/common.h>
#include <newt/stomp.h>
#include <newt/frame.h>
#include <newt/list.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

struct stomp_frame_info {
  char *name;
  int len;
};
static struct stomp_frame_info finfo_arr[] = {
  {"SEND",        4},
  {"SUBSCRIBE",   9},
  {"CONNECT",     7},
  {"STOMP",       5},
  {"ACK",         3},
  {"BEGIN",       5},
  {"COMMIT",      6},
  {"ABORT",       5},
  {"NACK",        4},
  {"UNSUBSCRIBE", 11},
  {"DISCONNECT",  10},
  {0},
};

frame_t *alloc_frame() {
  frame_t *ret;

  ret = (frame_t *)malloc(sizeof(frame_t));
  if(ret == NULL) {
    return NULL;
  }

  /* Initialize frame_t object */
  memset(ret->name, 0, FNAME_LEN);
  memset(ret->id, 0, FRAME_ID_LEN);

  INIT_LIST_HEAD(&ret->h_attrs);
  INIT_LIST_HEAD(&ret->h_data);
  INIT_LIST_HEAD(&ret->l_bucket);
  INIT_LIST_HEAD(&ret->l_transaction);
  INIT_LIST_HEAD(&ret->l_persistent);

  pthread_mutex_init(&ret->mutex_header, NULL);
  pthread_mutex_init(&ret->mutex_body, NULL);

  ret->sock = 0;
  ret->cinfo = NULL;
  ret->status = STATUS_BORN;
  ret->contentlen = -1;
  ret->has_contentlen = 0;
  ret->size = 0;

  ret->transaction_callback = NULL;
  ret->transaction_data = NULL;

  return ret;
}

void free_frame(frame_t *frame) {
  linedata_t *data, *l;

  /* delete header */
  if(! list_empty(&frame->h_attrs)) {
    list_for_each_entry_safe(data, l, &frame->h_attrs, list) {
      list_del(&data->list);
      free(data);
    }
  }

  /* delete body */
  if(! list_empty(&frame->h_data)) {
    list_for_each_entry_safe(data, l, &frame->h_data, list) {
      list_del(&data->list);
      free(data);
    }
  }

  if(frame->transaction_data != NULL) {
    free(frame->transaction_data);
  }

  free(frame);
}

static int ssplit(char *start, char *end, int *len, int is_body) {
  char *p;
  int count = 0;
  int ret = RET_SUCCESS;

  assert(start != NULL);
  assert(end != NULL);

  int is_separation = IS_BL(start);
  if(! is_body) {
    is_separation |= IS_NL(start);
  }

  if(is_separation) {
    *len = 0;

    return RET_SUCCESS;
  }

  do {
    count++;
    p = start + count;

    if(p >= end) {
      ret = RET_ERROR;
    }
  } while(! (*p == '\0' || (*p == '\n' && ! is_body) || p >= end || count >= LD_MAX));

  *len = count;

  return ret;
}

static int frame_setname(char *data, int len, frame_t *frame) {
  int i, ret = RET_ERROR;
  struct stomp_frame_info *finfo;

  assert(frame != NULL);

  for(i=0; finfo=&finfo_arr[i], finfo!=NULL; i++) {
    if(len < finfo->len) {
      continue;
    } else if(finfo->name == NULL) {
      break;
    }

    if(strncmp(data, finfo->name, finfo->len) == 0) {
      memcpy(frame->name, data, finfo->len);

      CLR(frame);
      SET(frame, STATUS_INPUT_HEADER);

      ret = RET_SUCCESS;
      break;
    }
  }

  return ret;
}

static void frame_finish(frame_t *frame) {
  // clear frame status
  CLR(frame);

  // set frame-id
  gen_random(frame->id, FRAME_ID_LEN);
}

static int get_contentlen(char *input) {
  return atoi(input + 15);
}

static int is_contentlen(char *input, int len) {
  if(len > 15 && strncmp(input, "content-length:", 15) == 0) {
    return RET_SUCCESS;
  }
  return RET_ERROR;
}

static int frame_update(frame_t *frame, char *line, int len) {
  int ret = 0;
  assert(frame != NULL);

  if(GET(frame, STATUS_BORN)) {
    if(frame_setname(line, len, frame) == RET_ERROR) {
      return -1;
    }
    debug("[frame_update] (%p) succeed in setting frame-name: %s", frame, frame->name);

    // '+1' means new-line character
    // when a frame is persistent, frame name and header is separated with a new-line character
    frame->size += (len + 1);
  } else if(GET(frame, STATUS_INPUT_HEADER)) {
    if(IS_NL(line)) {
      if(frame->contentlen > 0) {
        CLR(frame);
        SET(frame, STATUS_INPUT_BODY);

        // when a frame is persistent, header and body is separated with a new-line character
        frame->size += 1;

        return 0;
      } else if(frame->contentlen == 0) {
        frame_finish(frame);

        // when a frame is persistent, each frames are separated with NULL character ('\0')
        frame->size += 1;

        return 1;
      } else {
        while(! (IS_NL(line))) {
          line++;
          len -= 1;
        }
      }
    }

    if(len > 0) {
      if(is_contentlen(line, len) == RET_SUCCESS) {
        frame->contentlen = get_contentlen(line);
        debug("[frame_update] (%p) succeed in setting contentlen: %d", frame, frame->contentlen);
      }

      stomp_setdata(line, len, &frame->h_attrs, &frame->mutex_header);

      // '+1' means new-line character
      // when a frame is persistent, each headers are separated with a new-line character
      frame->size += (len + 1);
    }
  } else if(GET(frame, STATUS_INPUT_BODY)) {
    if(len > 0) {
      frame->has_contentlen += len;
      stomp_setdata(line, len, &frame->h_data, &frame->mutex_body);

      frame->size += len;

      debug("[frame_update] (%p) succeed in set body [%d/%d]", frame, frame->contentlen, frame->has_contentlen);
    }

    if(IS_BL(line)) {
      warn("[frame_update] (%p) got blank-line [contentlen:%d/%d]", frame, frame->contentlen, frame->has_contentlen);
    }

    if(frame->contentlen <= frame->has_contentlen) {
      frame_finish(frame);
      debug("[frame_update] (%p) succeed in parsing frame!!", frame);

      // when a frame is persistent, each frames are separated with NULL character ('\0')
      frame->size += 1;

      ret = 1;
    }
  }

  return ret;
}

//
// This parse STOMP frame from data streaming.
//
// args:
//  - frame_t *frame : frame object to make
//  - char *input    : raw data of frame
//  - int input_len  : length of raw data which is specified in 'input' argument
//  - int *offset    : offset of frame separation, which is set only in successful to parse frame
//
// returns:
//  - RET_SUCCESS : finished to parse frame
//  - RET_ERROR   : in the midst of parsing frame
int parse_frame(frame_t *frame, char *input, int input_len, int *offset) {
  static char *prev_buffer = NULL;
  static int prev_buffer_len = 0;
  char *curr, *next, *end, *line;
  int is_complete = RET_ERROR;

  assert(frame != NULL);
  *offset = -1;

  curr = input;
  end = (input + input_len);
  while(curr < end) {
    int line_len, ret;

    ret = ssplit(curr, end, &line_len, GET(frame, STATUS_INPUT_BODY));
    if(ret == RET_ERROR && GET(frame, STATUS_INPUT_BODY)) {
      char *data;
      if(prev_buffer != NULL) {
        data = (char *)realloc(prev_buffer, prev_buffer_len + line_len);
      } else {
        data = (char *)malloc(line_len);
      }

      memset(data + prev_buffer_len, 0, input_len);
      strncpy(data + prev_buffer_len, curr, line_len);

      prev_buffer = data;
      prev_buffer_len += input_len;

      break;
    }

    if(prev_buffer != NULL) {
      line = prev_buffer;

      strncpy(line + prev_buffer_len, curr, line_len);
      line_len += prev_buffer_len;

      free(prev_buffer);
      prev_buffer = NULL;
      prev_buffer_len = 0;
    } else {
      line = curr;
    }

    if(GET(frame, STATUS_INPUT_BODY)) {
      next = curr + line_len;
    } else {
      next = curr + line_len + 1;
    }

    ret = frame_update(frame, line, line_len);
    if(ret > 0) {
      is_complete = RET_SUCCESS;
      *offset = (next + 1) - input;

      break;
    }

    curr = next;
  }

  return is_complete;
}
