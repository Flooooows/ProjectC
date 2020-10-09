#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "socket.h"
#include "utils.h"

#define TAILLE_BUFFER_CMD 4099
#define TAILLE_PATH 4096
#define TAILLE_NOM_FICHIER 256
#define TAILLE_BUFFER 1024

int pipefd[2];

char serverIP[16];
int port;

void prompt() {
  char msgPrompt[] =
      "Commandes:"
      "\n   + <chemin d'un fichier> (Ajouter un fichier sur le serveur)"
      "\n   * num (Exécuter un programme de manière récurrente)"
      "\n   @ num (Exécuter un programme)"
      "\n   q (Déconnecter le client et libérer les ressources)"
      "\n > ";
  swrite(1, msgPrompt, sizeof(msgPrompt));
}

void printUsage() {
  char usage[] = "Utilisation: client <adr> <port> <delay>\n";
  swrite(1, usage, sizeof(usage));
}

void ajouterFichier(char* pathFichier);
void executerProgramme();
void filsRecurrent();

void minuterie(void* ptrDelay) {
  sclose(pipefd[0]);
  int delay = *(int*)ptrDelay;
  int battement = -1;
  int err = 1;
  while (err > 0) {
    sleep(delay);
    err = write(pipefd[1], &battement, sizeof(int));
  }
  sclose(pipefd[1]);
}

int main(int argc, char const* argv[]) {
  // arguments
  if (argc < 4) {
    printUsage();
    return 0;
  }
  strncpy(serverIP, argv[1], 16);
  port = atoi(argv[2]);
  int delay = atoi(argv[3]);

  spipe(pipefd);
  // création des fils
  int pid_rec = fork_and_run(filsRecurrent);
  int pid_minuterie = fork_and_run1(&minuterie, &delay);
  sclose(pipefd[0]);
  char buffer[TAILLE_BUFFER_CMD] = {0};
  while (1) {
    prompt();
    int cRead = read(0, buffer, TAILLE_BUFFER_CMD);
    buffer[cRead - 1] = '\0';
    char action = buffer[0];
    switch (action) {
      case '+': {
        char pathFichier[TAILLE_PATH];
        strcpy(pathFichier, buffer + 2);
        ajouterFichier(pathFichier);
      } break;
      case '*': {
        int numProg = atoi(buffer + 2);
        swrite(pipefd[1], &numProg, sizeof(int));
      } break;
      case '@': {
        int numProg = atoi(buffer + 2);
        executerProgramme(numProg);
      } break;
      case 'q': {
        sclose(pipefd[1]);
        kill(pid_rec, 9);
        kill(pid_minuterie, 9);
        swaitpid(pid_rec, NULL);
        swaitpid(pid_minuterie, NULL);
        return 0;
      } break;
      default: {
        const char cmdInconnu[] = "Commande inconnue\n";
        write(1, cmdInconnu, sizeof(cmdInconnu));
      }
    }
  }
}

void ajouterFichier(char* pathFichier) {
  // ouvrir le fichier en premier (vérifie si il existe)
  int fd = sopen2(pathFichier, O_RDONLY);

  int sockfd = initSocketClient(serverIP, port);

  // envoyer -1 au serveur pour annoncer un ajout
  int action = -1;
  swrite(sockfd, &action, sizeof(int));

  // convertir le chemin du fichier en nom de fichier
  char* nomFichier = strrchr(pathFichier, '/');
  if (nomFichier == NULL) {
    nomFichier = pathFichier;
  } else {
    nomFichier++;
  }
  int tailleNomFichier = strlen(nomFichier);

  // envoyer la taille du nom de fichier et le nom de fichier au serveur
  swrite(sockfd, &tailleNomFichier, sizeof(int));
  swrite(sockfd, nomFichier, tailleNomFichier);

  // lire le fichier et l'envoyer au serveur
  char buffer[TAILLE_BUFFER];
  int size;
  while ((size = sread(fd, buffer, TAILLE_BUFFER)) > 0) {
    swrite(sockfd, buffer, size);
  }
  sclose(fd);
  checkNeg(shutdown(sockfd, SHUT_WR), "error shutdown");

  // attente réponse serveur
  int numProgramme;
  sread(sockfd, &numProgramme, sizeof(int));
  printf("Num prog: %d\n", numProgramme);
  char bufferSocket[TAILLE_BUFFER];
  while ((size = sread(sockfd, bufferSocket, TAILLE_BUFFER)) > 0) {
    swrite(1, bufferSocket, size);
  }
  printf("\n");
}

void executerProgramme(int numProg) {
  int sockfd = initSocketClient(serverIP, port);
  int action = -2;
  swrite(sockfd, &action, sizeof(int));
  swrite(sockfd, &numProg, sizeof(int));
  int srvNumProg, srvEtatProg, srvTempsExec, srvCodeRet;
  sread(sockfd, &srvNumProg, sizeof(int));
  sread(sockfd, &srvEtatProg, sizeof(int));
  sread(sockfd, &srvTempsExec, sizeof(int));
  sread(sockfd, &srvCodeRet, sizeof(int));
  printf("\nNum prog: %d\nEtat prog: %d\nTemps exec: %d\nCode ret: %d\n",
         srvNumProg, srvEtatProg, srvTempsExec, srvCodeRet);
  char buffer[TAILLE_BUFFER];
  int sizeRd;
  while ((sizeRd = sread(sockfd, buffer, TAILLE_BUFFER))) {
    swrite(1, buffer, sizeRd);
  }
  printf("\n");
  sclose(sockfd);
}

void executerProgrammeRecurrent(void* numProg) {
  executerProgramme(*(int*)numProg);
  prompt();
}

void filsRecurrent() {
  sclose(pipefd[1]);
  int numProgs[100];
  pid_t pidExecRecurrent[100];
  int nbProg = 0;
  int ptr;
  while (sread(pipefd[0], &ptr, sizeof(int)) > 0) {
    if (ptr < 0) {
      for (int i = 0; i < nbProg; i++) {
        if (waitpid(pidExecRecurrent[i], NULL, WNOHANG) != 0) {
          pidExecRecurrent[i] =
              fork_and_run1(&executerProgrammeRecurrent, &(numProgs[i]));
        }
      }
    } else {
      numProgs[nbProg++] = ptr;
    }
  }
}