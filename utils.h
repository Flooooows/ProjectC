#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>
#include <sys/types.h>

//*****************************************************************************
// CHECK
//*****************************************************************************

void checkCond(bool cond, char* msg);

void checkNeg(int res, char* msg);

void checkNull(void* res, char* msg);

//*****************************************************************************
// FILE
//*****************************************************************************
int sopen1(char* path);

int sopen(char* path, int flag, int mode);

int sopen2(char* path, int flag);

void swrite(int fd, void* buff, int size);

int sread(int fd, void* buff, int size);

int nread(int fd, void* buff, int n);

void sclose(int fd);

//*****************************************************************************
// FORK
//*****************************************************************************

pid_t fork_and_run(void (*run)());

pid_t fork_and_run1(void (*handler)(void*), void* arg0);

pid_t fork_and_run2(void (*handler)(void*, void*), void* arg0, void* arg1);

void swaitpid(pid_t pid, int* status);

//*****************************************************************************
// PIPE
//*****************************************************************************

void spipe(int* fildes);

#endif