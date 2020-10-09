#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"

//*****************************************************************************
// CHECK
//*****************************************************************************
void checkCond(bool cond, char* msg) {
  if (cond) {
    perror(msg);
    exit(EXIT_FAILURE);
  }
}

void checkNeg(int res, char* msg) {
  checkCond(res < 0, msg);
}

void checkNull(void* res, char* msg) {
  checkCond(res == NULL, msg);
}

//*****************************************************************************
// FILE
//*****************************************************************************
int sopen(char* path, int flag, int mode) {
  int fd = open(path, flag, mode);
  checkNeg(fd, "Error OPEN");
  return fd;
}

int sopen2(char* path, int flag) {
  int fd = open(path, flag);
  checkNeg(fd, "Error OPEN");
  return fd;
}

int sopen1(char* path) {
  return sopen(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
}

void swrite(int fd, void* buff, int size) {
  int r = write(fd, buff, size);
  checkCond(r != size, "Error WRITE");
}

int sread(int fd, void* buff, int size) {
  int r = read(fd, buff, size);
  checkNeg(r, "Error READ");
  return r;
}

// READ EXACTLY n chars
int nread(int fd, void* buff, int n) {
  char* cbuff = (char*)buff;
  int s = sread(fd, cbuff, n);
  int i = s;
  while (s != 0 && i != n) {
    i += s;
    s = sread(fd, cbuff + i, n - i);
  }
  return s;
}

void sclose(int fd) {
  int r = close(fd);
  checkNeg(r, "Error CLOSE");
}

//*****************************************************************************
// FORK
//*****************************************************************************

pid_t fork_and_run(void (*handler)()) {
  int childId = fork();
  checkNeg(childId, "Error [fork_and_run]");

  // child process
  if (childId == 0) {
    (*handler)();
    exit(EXIT_SUCCESS);
  }

  return childId;
}

pid_t fork_and_run1(void (*handler)(void*), void* arg0) {
  int childId = fork();
  checkNeg(childId, "Error [fork_and_run]");

  // child process
  if (childId == 0) {
    (*handler)(arg0);
    exit(EXIT_SUCCESS);
  }

  return childId;
}

pid_t fork_and_run2(void (*handler)(void*, void*), void* arg0, void* arg1) {
  int childId = fork();
  checkNeg(childId, "Error [fork_and_run]");

  // child process
  if (childId == 0) {
    (*handler)(arg0, arg1);
    exit(EXIT_SUCCESS);
  }

  return childId;
}

void swaitpid(pid_t pid, int* status) {
  pid_t waitId = waitpid(pid, status, 0);
  checkNeg(waitId, "Error waitpid()");
}

//*****************************************************************************
// PIPE
//*****************************************************************************
void spipe(int* fildes) {
  int r = pipe(fildes);
  checkNeg(r, "Error pipe");
}
