#include <newt/daemon.h>
#include <newt/common.h>
#include <newt/connection.h>
#include <newt/stomp.h>
#include <newt/stomp_sending_worker.h>
#include <newt/stomp_management_worker.h>
#include <newt/persistent_worker.h>

#include <pthread.h>

enum ThreadWorkerName {
  CONNECTION_WORKER,
  CTRL_CONNECTION_WORKER,
  STOMP_MANAGEMENT_WORKER,
  PERSISTENT_WORKER,
  WorkerLength,
};

typedef struct thread_info {
  void *(*func)(void *);
  void *argument;
} thread_info_t;

int daemon_initialize(newt_config *config) {
  /* init processing for each protocol manager */
  if(stomp_init() == RET_ERROR) {
    perror("failed to initialize stomp bucket");
    return RET_ERROR;
  }

  if(transaction_init() == RET_ERROR) {
    perror("failed to initialize transaction_manager");
    return RET_ERROR;
  }

  if(initialize_sending_worker() == RET_ERROR) {
    return RET_ERROR;
  }

  if(initialize_persistent_worker(config) == RET_ERROR) {
    return RET_ERROR;
  }
  unpersist(); // get persistent frames and reinstate them

  return RET_SUCCESS;
}

int daemon_start(newt_config *config) {
  pthread_t worker_ids[WorkerLength];
  thread_info_t workers[] = {
    {connection_worker, config},
    {ctrl_connection_worker, config},
    {stomp_management_worker, NULL},
    {persistent_worker, NULL},
  };
  int i;

  if(config->loglevel != NULL) {
    set_logger(config->loglevel);
  }

  for(i=0; i<WorkerLength; i++) {
    pthread_create(&worker_ids[i], NULL, workers[i].func, workers[i].argument);
  }
  
  for(i=0; i<WorkerLength; i++) {
    pthread_join(worker_ids[i], NULL);
  }

  return RET_SUCCESS;
}
