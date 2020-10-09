#include "ipc.h"
#include <stddef.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#define PERM 0666

void down(int sem_id) {
  add_sem(-1, sem_id);
}

void up(int sem_id) {
  add_sem(1, sem_id);
}

int init_sem(int val, int sem_key) {
  int sem_id = semget(sem_key, 1, IPC_CREAT | PERM);
  checkNeg(sem_id, "Error semget");
  union semun arg;
  arg.val = val;
  int ret = semctl(sem_id, 0, SETVAL, arg);
  checkNeg(ret, "Error semctl");
  return sem_id;
}

int get_sem(int sem_key) {
  // GET THE SET OF SEMAPHORES.
  int sem_id = semget(sem_key, 0, PERM);
  checkNeg(sem_id, "Error semget");
  return sem_id;
}

void add_sem(int val, int sem_id) {
  struct sembuf sem;
  sem.sem_num = 0;
  sem.sem_op = val;
  sem.sem_flg = 0;
  int ret = semop(sem_id, &sem, 1);
  checkNeg(ret, "Error semop");
}

void del_sem(int sem_id) {
  int rv = semctl(sem_id, 0, IPC_RMID);
  checkNeg(rv, "Error semctl");
}

StructShm get_shm(int size, int shm_key) {
  int shm_id = shmget(shm_key, size, IPC_CREAT | PERM);
  checkNeg(shm_id, "Error shmget");
  void* ptr = shmat(shm_id, NULL, 0);
  checkCond(ptr == (void*)-1, "Error shmat");
  StructShm shm;
  shm.shm_id = shm_id;
  shm.ptr = ptr;
  return shm;
}

void sshmdt(void* ptr) {
  int r = shmdt(ptr);
  checkNeg(r, "Error shmdt");
}

void del_shm(int shm_id) {
  int r = shmctl(shm_id, IPC_RMID, NULL);
  checkNeg(r, "Error shmctl");
}