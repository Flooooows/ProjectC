#include <stdio.h>
#include <stdlib.h>

#include "ipc.h"
#include "progStats.h"

#define SHM_KEY 945
#define SEM_KEY 701

void printStats(ProgStats prog);

int main(int argc, char const* argv[]) {
  if (argc < 2) {
    printf("utilisation: gstat <id programme>\n");
    return 0;
  }
  // récupère l'id du programme demandé
  int id = atoi(argv[1]);

  int sem_id = get_sem(SEM_KEY);
  // récupère les stats dans la mémoire partagée
  Stats* allStats = (Stats*)(get_shm(sizeof(Stats), SHM_KEY)).ptr;
  ProgStats prog;
  int progExiste = 0;

  down(sem_id);
  if (id > 0 && allStats->tailleLogique >= id) {
    // récupère les stats du programme avec l'id récupéré
    prog = allStats->progStats[id - 1];
    progExiste = 1;
  }
  up(sem_id);

  if (progExiste) {
    printStats(prog);  // print les stats du programme
  } else {
    printf("Aucun programme ayant l'id %d n'a été trouvé\n", id);
  }

  return 0;
}

void printStats(ProgStats prog) {
  printf("Numéro du programme : %d\n", prog.numProg);
  printf("Nom du fichier : %s\n", prog.nomFichier);
  printf("Code compilation : %d\n", prog.erreurCompilation);
  printf("Nombre d'exécutions : %d\n", prog.nbExecutions);
  printf("Temps d'exécutions cumulées : %d\n", prog.tempsExecutionsCumulees);
}