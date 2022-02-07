#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int idQ;

WindowAdmin *w;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    idQ = msgget(CLE, 0);

    // Envoi d'une requete de login au serveur
    MESSAGE reponse;

    reponse.type = 1;
    reponse.expediteur = getpid();
    reponse.requete = LOGIN_ADMIN;
    strcpy(reponse.data1, "");
    strcpy(reponse.data2, "");
    strcpy(reponse.texte, "");
    
    if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
        perror("(ADMIN) Erreur lors de l'envoie de requete LOGIN_ADMIN");
        exit(1);
    }

    MESSAGE m;
    // Attente de la requête LOGIN_ADMIN serveur
    fprintf(stderr,"(ADMIN %d) Attente requete LOGIN_ADMIN...\n",getpid());
    if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1){
      perror("(ADMIN) Erreur de msgrcv\n");
      exit(1);
    }

    if(strcmp(m.data1, "KO") == 0){
      fprintf(stderr,"(ADMIN %d) Un Process Admin est déjà connecté\n",getpid());
      exit(0);
    }


    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
