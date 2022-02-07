#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <mysql.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "../protocole.h"

int idQ,idSem;
struct sembuf prep;

int main()
{
  fprintf(stderr,"=============================================================================\n");
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
    idQ = msgget(CLE, 0);
  
  // Recuperation de l'identifiant du sémaphore
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id du sémaphore\n",getpid());
    idSem = semget(CLE,0,0);

  MESSAGE m;
  char Nom[20];

  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Attente d'une requete...\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1){
    perror("(MODIFICATION) Erreur de msgrcv\n");
    exit(1);
  }
  strcpy(Nom, m.data1);

  //Preparation Reponse:
  MESSAGE reponse;

  reponse.type = m.expediteur;
  reponse.expediteur = getpid();
  reponse.requete = MODIF1;


  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateur est déjà en train de modifier)
  fprintf(stderr,"\t(MODIFICATION %d) Prise Non bloquante du sémaphore 0\n",getpid());
    prep.sem_num = 0; //Indice dans le tableau de semaphore
    prep.sem_op  = -1; //1 ou -1 au choix
    prep.sem_flg = IPC_NOWAIT; //Si process est suprimé en cours d'utilisation de la partie sensible opération annulé

  //Si Semop ne peut être prit: Envoi de la reponse négative au client
  if(semop(idSem, &prep, 1) == -1){
    
    strcpy(reponse.data1,"KO");
    strcpy(reponse.data2,"KO");
    strcpy(reponse.texte,"KO"); 

    fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());
    if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
      perror("(MODIFICATION) Erreur Envoi MODIF1");
      exit(1);
    }
    exit(0);
  }

  //Si non, il peut être prit, on continue normalement:
  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
  //mysql_real_connect(connexion,"localhost","bastinth","Passbastinth1_","Pourbastinth",0,0,0);
  //mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL){
    fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(), m.data1);

  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  
  //Préparation Requete SQL
  sprintf(requete, "SELECT motdepasse, gsm, email FROM UNIX_FINAL WHERE nom = '%s';", m.data1);

  mysql_query(connexion,requete);
  resultat = mysql_store_result(connexion);
  tuple = mysql_fetch_row(resultat); // user existe forcement

  strcpy(reponse.data1, tuple[0]);
  strcpy(reponse.data2, tuple[1]);
  strcpy(reponse.texte, tuple[2]); 

  // Construction et envoi de la reponse
  fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(MODIFICATION) Erreur Envoi MODIF1");
    exit(1);
  }

  // Attente de la requête MODIF2
  fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
  fprintf(stderr,"=============================================================================\n");
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1){
    perror("(MODIFICATION) Erreur de msgrcv\n");
    exit(1);
  }
  

  //MODIF2 reçus
  fprintf(stderr,"=============================================================================\n");
  //SI message != MODIF2
  if(m.requete != MODIF2){
    fprintf(stderr,"(MODIFICATION %d) Erreur, réception d'un mauvais messages\n",getpid());
    exit(1);
  }
  //SINON 
  // Mise à jour base de données
  fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),Nom);
  
  sprintf(requete, "UPDATE UNIX_FINAL SET motdepasse = '%s', gsm = '%s', email = '%s' WHERE nom = '%s';", m.data1, m.data2, m.texte, Nom);
  mysql_query(connexion,requete);
  
  //Si Erreur de Transaction
  if(mysql_errno(connexion) != 0)  fprintf(stderr,"(MODIFICATION %d) ERREUR MSQL %d: %s\n", getpid(), mysql_errno(connexion), mysql_error(connexion));
  
  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"\t(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  fprintf(stderr,"=============================================================================\n");
    prep.sem_num = 0; //Indice dans le tableau de semaphore
    prep.sem_op  = +1; //1 ou -1 au choix
    prep.sem_flg = 0; //Si process est suprimé en cours d'utilisation de la partie sensible opération annulé
    semop(idSem, &prep, 1); //1 signifie 1 semaphore
  exit(0);
}