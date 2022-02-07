#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "../protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd;

//Var Perso
int timeOut = 0;
int Time = 0;

char *ShmString; // Pointeur vers shm

//SIGNAUX
void handlerSIGUSR1(int sig);

int main()
{
  // Armement des signaux
  struct sigaction Action;
  // SIGUSR1
  Action.sa_handler = handlerSIGUSR1;
  sigemptyset(&Action.sa_mask);
  Action.sa_flags = 0;
  if(sigaction(SIGUSR1, &Action,NULL) == -1){
    fprintf(stderr,"(SERVEUR %d) Erreur de Sigaction\n",getpid());
  }
  
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  idQ = msgget(CLE, 0);

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  idShm = shmget(CLE, sizeof(char[200]), 0);

  // Attachement à la mémoire partagée en lecture/ecriture
  ShmString = (char *) shmat(idShm, NULL, 0);
  if(ShmString == (char *) -1){
    perror("(PUBLICITE) Erreur d'attachement mémoire");
    exit(0);
  }

  fprintf(stderr,"(PUBLICITE %d) Check File Exist\n",getpid());
  // Ouverture du fichier de publicité
  fd = open("publicites.dat",O_RDONLY);
  if(fd == -1){
    fprintf(stderr,"(PUBLICITE %d) File Not Exist, PAUSE\n",getpid());
    pause(); 
  }

  while(1){
  	PUBLICITE pub;
    //Si fin du fichierpub, on revient au début:
    if(read(fd, &pub, sizeof(PUBLICITE)) == 0){
      lseek(fd,0,SEEK_SET);
      read(fd, &pub, sizeof(PUBLICITE));
    }

    //Init Time:
    Time = pub.nbSecondes;

    // Ecriture en mémoire partagée
    strcpy(ShmString, pub.texte);

    // Envoi d'une requete UPDATE_PUB au serveur
    //Préparation Requète
      MESSAGE tmp;

      tmp.type = 1;
      tmp.expediteur = getpid();
      tmp.requete = UPDATE_PUB;
      strcpy(tmp.data1, "");
      strcpy(tmp.data2, "");
      strcpy(tmp.texte, "");
              
    //Envoie Requète UPDATE_PUB
    if( msgsnd(idQ, &tmp, sizeof(MESSAGE)-sizeof(long), 0) == -1){
      perror("(PUBLICITE) Erreur lors de l'envoie de requete UPDATE_PUB");
      exit(0);
    }

    sleep(pub.nbSecondes);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Handlers de signaux /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Handlers SIGUSR1
void handlerSIGUSR1(int sig){
  fprintf(stderr,"(PUBLICITE %d) Passage SIGUSR1",getpid());
    if((fd = open("publicites.dat", O_RDONLY)) == -1){
      perror("(PUBLICITE) Erreur de open SIGUSR1");
      exit(1);
    }  

}
