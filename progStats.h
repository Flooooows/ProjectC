#include <stdbool.h>

#define MAX_NOM_FICHIER 256
#define NB_PROG_STATS 1000

typedef struct {
  int numProg;
  char nomFichier[MAX_NOM_FICHIER];
  bool erreurCompilation;
  int nbExecutions;
  int tempsExecutionsCumulees;
} ProgStats;

typedef struct {
  int tailleLogique;
  int nbExecGlobal;
  ProgStats progStats[NB_PROG_STATS];
} Stats;