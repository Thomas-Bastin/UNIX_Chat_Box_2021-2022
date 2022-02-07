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

int main(){
  
  fprintf(stderr,"=============================================================================\n");
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"\t(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
    idQ = msgget(CLE, 0);

  // Recuperation de l'identifiant du sémaphore
  fprintf(stderr,"\t(CONSULTATION %d) Recuperation de l'id du sémaphore\n",getpid());
    idSem = semget(CLE,0,0);



  // Lecture de la requête CONSULT
  fprintf(stderr,"\t(CONSULTATION %d) Lecture requete CONSULT\n",getpid());

  //Recuperation MSG:
  MESSAGE m;
  fprintf(stderr,"\t(CONSULTATION %d) Attente d'une requete...\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1){
    perror("\t(CONSULTATION) Erreur de msgrcv\n");
    exit(1);
  }

  // Construction de la reponse
  MESSAGE reponse;

  reponse.type = m.expediteur;
  reponse.expediteur = getpid();
  reponse.requete = CONSULT;


  // Tentative de prise bloquante du semaphore 0
  fprintf(stderr,"\t(CONSULTATION %d) Prise bloquante du sémaphore 0\n",getpid());
    
    prep.sem_num = 0; //Indice dans le tableau de semaphore
    prep.sem_op  = -1; //1 ou -1 au choix
    prep.sem_flg = 0; //Si process est suprimé en cours d'utilisation de la partie sensible opération annulé
    if(semop(idSem, &prep, 1) == -1) //1 signifie 1 semaphore
    {
      perror("(CONSULTATION) semopfail");
      exit(1);
    }

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"\t(CONSULTATION %d) Connexion à la BD\n",getpid());
  //mysql_real_connect(connexion,"localhost","bastinth","Passbastinth1_","Pourbastinth",0,0,0);
  //mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL){
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"\t(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  
  //Préparation Requete SQL
  sprintf(requete, "SELECT gsm, email FROM UNIX_FINAL WHERE nom = '%s';", m.data1);
  
  //ExecRequète
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);

  //Traitement Résultat SQL
  if(resultat == NULL) exit(0);

  tuple = mysql_fetch_row(resultat); //Ce tuple comprend un seul champ mdp 
                            
  if(tuple == NULL){  //résultat NULL, Nom Inconnu dans la BDD
    fprintf(stderr,"\t(CONSULTATION %d) Nom Inconnu (%s)\n",getpid(),m.data1);
    strcpy(reponse.data1, "KO");    //Nom pas enregistré:   Préparation requète Login KO  
  }
  else{ //Si pas NULL, on vérifié que MDP est bon.                        
    fprintf(stderr,"\t(CONSULTATION %d) Nom Trouvé (%s)\n",getpid(),m.data1);
    strcpy(reponse.data1, "OK");    //Nom trouvé: Préparation requète Login OK  
    strcpy(reponse.data2, tuple[0]);
    strcpy(reponse.texte, tuple[1]);               
  }

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"\t(CONSULTATION %d) Libération du sémaphore 0\n",getpid());
    prep.sem_num = 0; //Indice dans le tableau de semaphore
    prep.sem_op  = +1; //1 ou -1 au choix
    prep.sem_flg = 0; //Si process est suprimé en cours d'utilisation de la partie sensible opération annulé
    if(semop(idSem, &prep, 1) == -1) //1 signifie 1 semaphore
    {
      perror("(CONSULTATION) semopfail");
      exit(1);
    }
  
  //Envoie de la réponse:
  fprintf(stderr,"\t(CONSULTATION %d) Envoie requete CONSULT\n",getpid());
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CONSULTATION) Erreur lors de l'envoie de requete Consultation");
  }

  kill(m.expediteur, SIGUSR1);

  fprintf(stderr,"=============================================================================\n");
  
  exit(0);
}