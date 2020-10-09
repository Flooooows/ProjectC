#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ipc.h"
#include "progStats.h"
#include "socket.h"
#include "utils.h"

#define SHM_KEY 945
#define SEM_KEY 701

#define PERM 0666
#define BUFFER 100
#define TAILLE_NOM_FICHIER 256
#define TAILLE_ID_PROG 10
#define PROG_FOLDER "progs"

struct sigaction base_sa;

void handlerSockfd(void* ptrSockfd);
void actionAjouter(int newsockfd, int sem_id, Stats* allStats);
void actionExecuter(int newsockfd, int sem_id, Stats* allStats);

void executeProg(void* ptrNumProg, void* ptrNomFichier) {
  int numProg = *(int*)ptrNumProg;
  char nom[TAILLE_ID_PROG + sizeof(PROG_FOLDER)];
  sprintf(nom, "%s/%d", PROG_FOLDER, numProg);
  execl(nom, ptrNomFichier, NULL);
  perror("Error execl execute");
}

void compileProg(void* ptrPath, void* ptrPathExec) {
  char* path = (char*)ptrPath;
  char* pathExec = (char*)ptrPathExec;
  execl("/usr/bin/gcc", "gcc", "-o", pathExec, path, NULL);
  perror("Error execl compile");
}

long now() {
  struct timeval tv;

  int res = gettimeofday(&tv, NULL);
  checkNeg(res, "Error gettimeofday");

  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char const* argv[]) {
  if (argc < 2) {
    printf("Utilisation: server [port]\n");
    exit(0);
  }

  // initialise le serveur sur le port passé en paramètre
  int port = atoi(argv[1]);
  int sockfd = initSocketServer(port);

  // Créer le répertoire de code si il n'existe pas
  mkdir(PROG_FOLDER, PERM);

  // Ignorer le signal SIGCHLD pour empêcher les processus zombies
  struct sigaction sa = {{0}};
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, &base_sa);

  while (1) {
    int newsockfd = accept(sockfd, NULL, NULL);
    checkNeg(newsockfd, "Error accept");
    fork_and_run1(&handlerSockfd, &newsockfd);
    sclose(newsockfd);
  }

  return 0;
}

void handlerSockfd(void* ptrSockfd) {
  int newsockfd = *(int*)ptrSockfd;

  // Ne pas ignorer le signal SIGCHLD
  sigaction(SIGCHLD, &base_sa, NULL);

  sigset_t newmask, oldmask;
  int ret = sigemptyset(&newmask);
  checkNeg(ret, "error sigemptyset");
  ret = sigaddset(&newmask, SIGINT);
  checkNeg(ret, "error sigaddset");

  // Bloquer le signal SIGINT
  ret = sigprocmask(SIG_BLOCK, &newmask, &oldmask);
  checkNeg(ret, "error sigprocset");

  int action;
  sread(newsockfd, &action, sizeof(int));

  // Acces mémoire partagée
  int sem_id = get_sem(SEM_KEY);
  // récupère les stats dans la mémoire partagée
  Stats* allStats = (Stats*)(get_shm(sizeof(Stats), SHM_KEY)).ptr;

  if (action == -1) {
    actionAjouter(newsockfd, sem_id, allStats);
  } else if (action == -2) {
    actionExecuter(newsockfd, sem_id, allStats);
  }
  sshmdt(allStats);
  sclose(newsockfd);
}

void actionAjouter(int newsockfd, int sem_id, Stats* allStats) {
  int checkret, nbCaracsFichier;
  sread(newsockfd, &nbCaracsFichier, sizeof(int));
  char nomFichier[TAILLE_NOM_FICHIER];
  sread(newsockfd, &nomFichier, nbCaracsFichier);
  nomFichier[nbCaracsFichier] = '\0';

  // Début section critique
  down(sem_id);

  int idFichier = allStats->tailleLogique + 1;
  char nomExec[TAILLE_ID_PROG + sizeof(PROG_FOLDER) + 2];
  char nom[TAILLE_ID_PROG + sizeof(PROG_FOLDER) + 5];
  sprintf(nomExec, "%s/%d", PROG_FOLDER, idFichier);
  sprintf(nom, "%s.c", nomExec);

  int fd;
  char buffRd[BUFFER];
  int nbCharRd;
  fd = sopen1(nom);

  // Lecture du fichier recu et ecriture dans le fichier créé
  while ((nbCharRd = read(newsockfd, buffRd, BUFFER))) {
    if ((write(fd, buffRd, nbCharRd) != nbCharRd)) {
      perror("Error writing fd from newsockfd");
    }
  }
  sclose(fd);
  swrite(newsockfd, &idFichier, sizeof(int));

  // Gestion des erreurs de compilation
  int fdError = sopen("res_compile.txt", O_CREAT | O_RDWR | O_TRUNC, PERM);
  // Redirection
  int stderr_copy = dup(2);
  checkNeg(stderr_copy, "ERROR dup");
  checkret = dup2(fdError, 2);
  checkNeg(checkret, "Error dup2");

  // Compilation du programme
  int pidCompile = fork_and_run2(&compileProg, nom, nomExec);
  int statusCompile;
  swaitpid(pidCompile, &statusCompile);

  // Retablir strerr
  checkret = dup2(stderr_copy, 2);
  checkNeg(checkret, "ERROR dup");
  sclose(stderr_copy);

  checkNeg(lseek(fdError, 0, SEEK_SET), "error lseek");
  // Lecture du fichier de compilation et envoie de message au client
  char buffRd1[BUFFER];
  while ((nbCharRd = sread(fdError, buffRd1, BUFFER))) {
    if ((write(newsockfd, buffRd1, nbCharRd) != nbCharRd)) {
      perror("Error writing on newsock from fdError");
    }
  }
  ProgStats* progStats = allStats->progStats + allStats->tailleLogique;
  strcpy(progStats->nomFichier, nomFichier);
  progStats->numProg = idFichier;
  progStats->erreurCompilation = (statusCompile == 0) ? 0 : 1;
  progStats->nbExecutions = 0;
  progStats->tempsExecutionsCumulees = 0;
  allStats->tailleLogique++;
  // Fin section critique
  up(sem_id);
}

void actionExecuter(int newsockfd, int sem_id, Stats* allStats) {
  int checkret;
  int numProg;
  ProgStats* prog;
  int etat = 0;
  int tpsExec = 0;
  int statusExec;
  int exitCode = 0;
  sread(newsockfd, &numProg, sizeof(int));

  down(sem_id);

  // Si le programme existe
  if (numProg > 0 && allStats->tailleLogique >= numProg) {
    // récupère les stats du programme avec l'id récupéré
    prog = allStats->progStats + numProg - 1;
    // Gestion de l'affichage
    int numExecution = ++(allStats->nbExecGlobal);
    char nomFichier[TAILLE_ID_PROG + sizeof(PROG_FOLDER)];
    strcpy(nomFichier, prog->nomFichier);
    bool errCompile = prog->erreurCompilation;
    up(sem_id);
    if (!errCompile) {
      char nomFichierExec[TAILLE_ID_PROG + 4];
      sprintf(nomFichierExec, PROG_FOLDER "/%d.txt", numExecution);
      int fdExec = sopen(nomFichierExec, O_CREAT | O_RDWR | O_TRUNC, PERM);
      // Redirection
      int stdout_copy = dup(1);
      checkNeg(stdout_copy, "ERROR dup");
      checkret = dup2(fdExec, 1);
      checkNeg(checkret, "Error dup2");

      long t1 = now();
      // Execution du programme
      int pidExec = fork_and_run2(&executeProg, &numProg, nomFichier);
      swaitpid(pidExec, &statusExec);
      long t2 = now();

      // Si tout va bien
      tpsExec = (int)t2 - t1;
      down(sem_id);
      prog->nbExecutions++;
      prog->tempsExecutionsCumulees += tpsExec;
      up(sem_id);
      etat = WIFEXITED(statusExec) ? 1 : 0;
      exitCode = WEXITSTATUS(statusExec);

      swrite(newsockfd, &numProg, sizeof(int));
      swrite(newsockfd, &etat, sizeof(int));
      swrite(newsockfd, &tpsExec, sizeof(int));
      swrite(newsockfd, &exitCode, sizeof(int));

      // Retablir redirection
      checkret = dup2(stdout_copy, 1);
      checkNeg(checkret, "ERROR dup");
      sclose(stdout_copy);

      // Lecture du fichier d'execution et envoie de message au client
      int nbCharRd2;
      char buffRd2[BUFFER];
      checkNeg(lseek(fdExec, 0, SEEK_SET), "error lseek");
      while ((nbCharRd2 = sread(fdExec, buffRd2, BUFFER))) {
        if ((write(newsockfd, buffRd2, nbCharRd2) != nbCharRd2)) {
          perror("Error writing on newsock from fdExec");
        }
      }
    } else {
      // Programme ne compile pas
      etat = -1;
      swrite(newsockfd, &numProg, sizeof(int));
      swrite(newsockfd, &etat, sizeof(int));
      swrite(newsockfd, &tpsExec, sizeof(int));
      swrite(newsockfd, &exitCode, sizeof(int));
    }
  } else {  // Programme n'existe pas
    up(sem_id);
    etat = -2;
    tpsExec = -1;
    statusExec = -1;
    swrite(newsockfd, &numProg, sizeof(int));
    swrite(newsockfd, &etat, sizeof(int));
    swrite(newsockfd, &tpsExec, sizeof(int));
    swrite(newsockfd, &statusExec, sizeof(int));
  }
}
