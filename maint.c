#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"
#include "progStats.h"

#define SHM_KEY 945
#define SEM_KEY 701

void creerRepertoireExecution();
void detruireRepertoireExecution();
void reserverReportoireExecution(int secondes);

void printUsage() {
  printf("utilisation:\n");
  printf("maint 1: créer répertoire exécution\n");
  printf("maint 2: detruire répertoire exécution\n");
  printf("maint 3 <secondes>: réserver répertoire exécution\n");
}

int main(int argc, char const* argv[]) {
  if (argc <= 1) {
    printUsage();
    exit(0);
  }
  int type = atoi(argv[1]);

  switch (type) {
    case 1:
      creerRepertoireExecution();
      break;
    case 2:
      detruireRepertoireExecution();
      break;
    case 3:
      if (argc < 3) {
        printUsage();
      } else {
        reserverReportoireExecution(atoi(argv[2]));
      }
      break;
    default:
      printUsage();
  }

  return 0;
}

void creerRepertoireExecution() {
  init_sem(1, SEM_KEY);
  Stats* allStats = (Stats*)(get_shm(sizeof(Stats), SHM_KEY)).ptr;
  allStats->nbExecGlobal = 0;
  allStats->tailleLogique = 0;
  sshmdt(allStats);
  printf("Répertoire d'execution cree!\n");
}

void detruireRepertoireExecution() {
  StructShm shm = get_shm(sizeof(Stats), SHM_KEY);
  int shm_id = shm.shm_id;
  del_shm(shm_id);
  int sem_id = get_sem(SEM_KEY);
  del_sem(sem_id);
  sshmdt(shm.ptr);
  printf("Répertoire d'execution detruit!\n");
}

void reserverReportoireExecution(int secondes) {
  printf("Bloquage du répertoire d'execution pendant %d secondes:\n", secondes);
  int sem_id = get_sem(SEM_KEY);
  printf("Demande de bloquage\n");
  down(sem_id);
  printf("Début bloquage, débloquage dans %d secondes...\n", secondes);
  sleep(secondes);
  printf("Débloquage\n");
  up(sem_id);
}