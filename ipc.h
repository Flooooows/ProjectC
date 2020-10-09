#ifndef _IPC_H_
#define _IPC_H_

#include <sys/types.h>
#include "utils.h"

union semun {
  int val;               /* Value for SETVAL */
  struct semid_ds* buf;  /* Buffer for IPC_STAT, IPC_SET */
  unsigned short* array; /* Array for GETALL, SETALL */
  struct seminfo* __buf; /* Buffer for IPC_INFO (Linux-specific) */
};

//*****************************************************************************
// SEMAPHORES
//*****************************************************************************

void down(int sem_id);

void up(int sem_id);

int init_sem(int val, int sem_key);

int get_sem(int shm_key);

void add_sem(int val, int sem_id);

void del_sem(int sem_id);

//*****************************************************************************
// SHARED MEMORY
//*****************************************************************************

typedef struct {
  int shm_id;
  void* ptr;
} StructShm;

// init
StructShm get_shm(int size, int shm_key);

// reset
void sshmdt(void* ptr);

void del_shm(int shm_id);

#endif