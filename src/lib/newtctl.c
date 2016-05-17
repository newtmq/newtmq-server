#include <newt/newtctl.h>
#include <newt/common.h>
#include <newt/logger.h>

#include <assert.h>
#include <stdlib.h>
#include <msgpack.h>

#define RECV_BUFSIZE (1<<13)

#define UNPACK_INT(OBJ, PTR) (*PTR = (int)OBJ.via.i64)
#define UNPACK_STR(OBJ, PTR) (memcpy(PTR, OBJ.via.raw.ptr, OBJ.via.raw.size))

struct ctl_command_handler_t {
  int command;
  int (*handler)(newtctl_t *);
};

struct ctl_command_handler_t handlers[] = {
  {NEWTCTL_LIST_QUEUES, newtctl_list_queues},
  {0},
};

static char *pack(newtctl_t *data, int *len) {
  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  char *ret = NULL;

  if(data != NULL) {
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, NEWTCTL_MEMBERS);
    msgpack_pack_int(&pk, data->command);
    msgpack_pack_int(&pk, data->status);
    msgpack_pack_raw(&pk, sizeof(data->context));
    msgpack_pack_raw_body(&pk, data->context, sizeof(data->context));

    ret = (char *)malloc(sbuf.size);
    if(ret != NULL) {
      *len = sbuf.size;
      memcpy(ret, sbuf.data, sbuf.size);
    }
    msgpack_sbuffer_destroy(&sbuf);
  }

  return ret;
}

static int unpack(void *ptr, int size, newtctl_t *data) {
  msgpack_zone mempool;
  msgpack_object root;
  int ret = RET_ERROR;

  msgpack_zone_init(&mempool, RECV_BUFSIZE);
  msgpack_unpack(ptr, size, NULL, &mempool, &root);

  assert(data != NULL);

  if(root.type == MSGPACK_OBJECT_ARRAY && root.via.array.size == NEWTCTL_MEMBERS) {
    UNPACK_INT(root.via.array.ptr[0], &data->command);
    UNPACK_INT(root.via.array.ptr[1], &data->status);
    UNPACK_STR(root.via.array.ptr[1], data->context);

    ret = RET_SUCCESS;
  }
  msgpack_zone_destroy(&mempool);

  return ret;
}

void *newtctl_worker(struct conninfo *cinfo) {
  char *buf;
  newtctl_t *cobj;

  assert(cinfo != NULL);

  cobj = (newtctl_t *)malloc(sizeof(newtctl_t));
  buf = (char *)malloc(RECV_BUFSIZE);
  if(buf != NULL && cobj != NULL) {
    int len;

    while(1) {
      memset(buf, 0, RECV_BUFSIZE);
      if((len = recv(cinfo->sock, buf, RECV_BUFSIZE, 0)) <= 0) {
        break;
      }
  
      if(unpack(buf, len, cobj) == RET_ERROR) {
        warn("[newtctl_worker] unpack is failed. input data is invalid");
        continue;
      }
  
      struct ctl_command_handler_t *curr = handlers;
      for(curr = handlers; curr->command > 0; curr++) {
        if(curr->command == cobj->command) {
          cobj->status = curr->handler(cobj);
          break;
        }
      }
  
      char *send_data = pack(cobj, &len);
      if(send_data != NULL) {
        send(cinfo->sock, send_data, len, 0);
  
        free(send_data);
      }
    }
  }

  free(buf);
  free(cobj);

  return NULL;
}
