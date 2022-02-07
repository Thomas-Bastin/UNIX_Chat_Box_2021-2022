#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include <errno.h>
#include "../protocole.h" // contient la cle et la structure d'un message

int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;

void afficheTab();

//Handler Perso
void handlerSIGINT(int sig);
void handlerSIGCHLD(int sig);



MYSQL* connexion;

int main()
{
  // Connection à la BD
  connexion = mysql_init(NULL);
  //mysql_real_connect(connexion,"localhost","bastinth","Passbastinth1_","Pourbastinth",0,0,0);
  //mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL){
    fprintf(stderr,"(SERVEUR %d) Erreur de connexion à la base de données...\n",getpid());
    kill(getpid(),SIGINT); //Exit propre
  }


  // Armement des signaux
  struct sigaction Action;

  // SIGINT
  Action.sa_handler = handlerSIGINT;
  sigemptyset(&Action.sa_mask);
  Action.sa_flags = 0;
  if(sigaction(SIGINT, &Action,NULL) == -1){
    fprintf(stderr,"(SERVEUR %d) Erreur de Sigaction\n",getpid());
  }

  //SIGCHLD
  Action.sa_handler = handlerSIGCHLD;
  sigemptyset(&Action.sa_mask);
  Action.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &Action,NULL) == -1){
    fprintf(stderr,"(SERVEUR %d) Erreur de Sigaction\n",getpid());
  }
  


  // creation de la file de messages
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0600)) == -1){  // CLE definie dans protocole.h
    perror("(SERVEUR) Erreur de msgget()");
    kill(getpid(),SIGINT); //Exit propre
  }

  // creation de la mémoire partagée
  fprintf(stderr,"(SERVEUR %d) Creation de la mémoire partagée\n",getpid());
  if ((idShm = shmget(CLE, sizeof(char[200]),IPC_CREAT | IPC_EXCL | 0600)) == -1){  // CLE definie dans protocole.h
    perror("(SERVEUR) Erreur de shmget()");
    kill(getpid(),SIGINT); //Exit propre
  }

  // creation Semaphore (MUTEX)
  fprintf(stderr,"(SERVEUR %d) Creation du Sémaphore\n",getpid());
  if ((idSem = semget(CLE, 1,IPC_CREAT | IPC_EXCL | 0600)) == -1){  // CLE definie dans protocole.h
    perror("(SERVEUR) Erreur de semget()");
    kill(getpid(),SIGINT); //Exit propre
  }
  
  // Init Semaphore (MUTEX)
  if(semctl(idSem, 0,SETVAL,1) == -1){
    perror("(SERVEUR) Erreur de semctl()");
    kill(getpid(),SIGINT); //Exit propre
  }



  // Initilisation de la table de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++){
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }

  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();




  // Creation du processus Publicite
  tab->pidPublicite = fork();

  //Process Fils: on fera un wait lors du sigint, mais en soit on pourrais le laisser tourner et laisser l'os faire sont boulot.
  if(tab->pidPublicite == 0){
    execl("./Publicite.app","Publicite.app",NULL);
  }



  
  //Variables Prof
  int i,k,j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  //Mes Variables
  int test = 0;
  char Nom[50];
  int modifier, Accepter, Refuser;
  int Exp;
  int pidConsultation;
  int fd;

  while(1){
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1){
      if(errno == EINTR) continue; //Obligatoire, car msgrcv ce coupe malgré SA_RESTART, lors d'un EINTR
      perror("(SERVEUR) Erreur de msgrcv");
      kill(getpid(),SIGINT); //Exit propre
    }

    fprintf(stderr,"\n\n\n=============================================================================\n");
    switch(m.requete)
    {
      case CONNECT:   fprintf(stderr,"\t\t(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      
                      test = 0;
                      for(int i = 0; i<6 && test != 1; i++){
                        if(tab->connexions[i].pidFenetre == 0){
                          tab->connexions[i].pidFenetre = m.expediteur;
                          test = 1;
                        }
                      }

                      if(test == 0){
                          fprintf(stderr,"(SERVEUR %d) Trop de Client connecte\n",getpid());
                          //Si plus que 6 clients ouvert.if(m.type == 1){
                          kill(m.expediteur, SIGKILL);
                      }
                      break;

      case DECONNECT: fprintf(stderr,"\t\t(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      
                      test = 0;
                      for(int i = 0; i<6 && test != 1; i++){
                        if(tab->connexions[i].pidFenetre == m.expediteur){
                          tab->connexions[i].pidFenetre = 0;
                          strcpy(tab->connexions[i].nom, "");
                          tab->connexions[i].autres[0] = 0;
                          tab->connexions[i].autres[1] = 0;
                          tab->connexions[i].autres[2] = 0;
                          tab->connexions[i].autres[3] = 0;
                          tab->connexions[i].autres[4] = 0;
                          tab->connexions[i].pidModification = 0;

                          test = 1;
                        }
                      }

                      break; 

      case LOGIN:     fprintf(stderr,"\t\t(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);

                      //Vérification Utilisateur pas déjà connecté:
                      test = 0;
                      for(int i = 0; i<6 && test != 1; i++){
                        if(strcmp(tab->connexions[i].nom, m.data1) == 0){
                          test = 1;
                        } 
                      }

                      //Si déjà connecté on envoie le msg avec KO + DUP
                      if(test == 1){ 
                        fprintf(stderr,"\t\t(SERVEUR %d) Compte Duplique (%s)\n",getpid(),m.data1);
                        strcpy(reponse.data1, "KO");  //Duplication de compte:  Préparation msg Login ko
                        strcpy(reponse.data2, "DUP");
                      }
                      else{ 
                          //Si non on recherche dans la bdd une correspondance au mdp + login
                          fprintf(stderr,"\t\t(SERVEUR %d) Consultation en BD (%s)\n",getpid(),m.data1);
                              
                          //Préparation Requete SQL
                          sprintf(requete, "SELECT motdepasse FROM UNIX_FINAL WHERE nom = '%s';", m.data1);

                          //Requete SQL
                          mysql_query(connexion,requete);
                          resultat = mysql_store_result(connexion);

                          //Traitement Résultat SQL
                          if(resultat == NULL) kill(getpid(),SIGINT); //Exit propre

                          tuple = mysql_fetch_row(resultat); //Ce tuple comprend un seul champ mdp 
                            
                          if(tuple == NULL){  //résultat NULL, Nom Inconnu dans la BDD
                            fprintf(stderr,"\t\t(SERVEUR %d) Nom Inconnu (%s)\n",getpid(),m.data1);
                            strcpy(reponse.data1, "KO");    //Nom pas enregistré:   Préparation requète Login ko
                            strcpy(reponse.data2, "USER");
                          }
                          else{ //Si pas NULL, on vérifié que MDP est bon.                        
                            if(strcmp(tuple[0], m.data2) == 0){   //Test MDP BDD == MDP entré par l'utilisateur
                              test = 0;
                              for(int i = 0; i<6 && test != 1; i++){  //Bon MDP, on l'ajoute a la table de connexion  (premier champ qui vaut 0)
                                  if(tab->connexions[i].pidFenetre == m.expediteur){
                                    strcpy(tab->connexions[i].nom, m.data1);
                                    test = 1;
                                  } 
                              }
                              strcpy(reponse.data1, "OK");  //Préparation msg Login ok
                            }
                            else{
                                  fprintf(stderr,"\t\t(SERVEUR %d) MotDePasse Incorrecte (%s)\n",getpid(),m.data1);
                                  strcpy(reponse.data1, "KO");  //Mot de Passe incorecte:  Préparation msg Login ko
                                  strcpy(reponse.data2, "MDP");
                            }                        
                          }
                      }


                      //Préparation ack Login
                      reponse.type = m.expediteur;
                      reponse.expediteur = 1;
                      reponse.requete = LOGIN;
                      strcpy(reponse.texte, "");

                      //Envoi ack login
                      if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                        perror("(SERVEUR) Erreur lors de l'ack Login USER");
                        kill(getpid(),SIGINT); //Exit propre
                      }

                      //Envoi SIGUSR1
                      kill(m.expediteur, SIGUSR1);

                      //Partie 2 Envoie des messages aux autres pour dire qu'il est connecté et le serveur envoie les users déjà encodé au nouvel user
                      if(strcmp(m.data1, "") != 0 && strcmp(reponse.data1,"OK") == 0){
                                               
                        //Préparation des ADD_USERs
                        reponse.expediteur = 1;
                        reponse.requete = ADD_USER;
                        strcpy(reponse.data2, "");
                        strcpy(reponse.texte, "");


                        //Envoi ADD_USER au nouvel USER (qui vient de login) pour chaque utilisateur déjà présent dans la table de connexion
                        reponse.type = m.expediteur;
                        for(int i = 0; i<6 ; i++){
                          if(strcmp(tab->connexions[i].nom, "") != 0 && strcmp(tab->connexions[i].nom, m.data1) != 0){                              

                              strcpy(reponse.data1, tab->connexions[i].nom);
                              if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                                perror("(SERVEUR) Erreur lors du ADD_USER qui vient de login");
                                kill(getpid(), SIGKILL);//Exit safe
                              }
                              //Envoi SIGUSR1 aux USERs devant ajouté le nouvel USER qui vient de login
                              kill(m.expediteur, SIGUSR1);
                          }
                        }


                        //Envoi ADD_USER a chaque USER déjà connecté (sauf lui même) pour prévenir de l'ajout d'un nouvel USER joignable (qu'on vient de login)
                        strcpy(reponse.data1, m.data1);
                        for(int i = 0; i<6 ; i++){
                          if(strcmp(tab->connexions[i].nom, "") != 0 && strcmp(tab->connexions[i].nom, m.data1) != 0){

                              reponse.type = tab->connexions[i].pidFenetre;
                              if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                                perror("(SERVEUR) Erreur lors du ADD_USER déjà connecté");
                                kill(getpid(), SIGKILL); //ExitSafe
                              }
                              //Envoi SIGUSR1 aux USERs devant ajouté le nouvel USER qui vient de login
                              kill(tab->connexions[i].pidFenetre, SIGUSR1);
                          }
                        }
                      }

                      break; 

      case LOGOUT:    fprintf(stderr,"\t\t(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);

                      //Suppression dans la table des connexions de l'utilisateur et ses 'accepter / refuser'
                      test = 0;
                      for(int i = 0; i<6 && test != 1; i++){
                        if(tab->connexions[i].pidFenetre == m.expediteur){
                          strcpy(Nom, tab->connexions[i].nom);
                          strcpy(tab->connexions[i].nom, "");
                          tab->connexions[i].autres[0] = 0;
                          tab->connexions[i].autres[1] = 0;
                          tab->connexions[i].autres[2] = 0;
                          tab->connexions[i].autres[3] = 0;
                          tab->connexions[i].autres[4] = 0;
                          tab->connexions[i].pidModification = 0;
                          test = 1;
                        } 
                      }
                      
                      //Suppression dans les autres tables du pidFenetre de la connexion supprimé
                      for (int i=0 ; i<6 ; i++){
                        for (int j = 0; j < 5; j++){
                          if(tab->connexions[i].autres[j] == m.expediteur){
                                tab->connexions[i].autres[j] = 0;
                          }
                        }
                      }
                      

                      //Partie 2 Envoie des messages aux autres pour dire qu'il est déconnecté
                      //Préparation des REMOVE_USER
                      reponse.expediteur = 1;
                      reponse.requete = REMOVE_USER;
                      strcpy(reponse.data1, Nom);
                      strcpy(reponse.data2, "");
                      strcpy(reponse.texte, "");

                                              
                        //Envoi REMOVE_USER a chaque USER déjà connecté (sauf lui même) pour prévenir du logout de l'utilisateur.
                        for(int i = 0; i<6 ; i++){
                          if(strcmp(tab->connexions[i].nom, "") != 0 && strcmp(tab->connexions[i].nom, Nom) != 0){
                              
                              //Prépare le msg
                              reponse.type = tab->connexions[i].pidFenetre;

                              if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                                perror("(SERVEUR) Erreur lors du REMOVE_USER");
                                kill(getpid(),SIGINT); //Exit propre
                              }
                              //Envoi SIGUSR1 aux USERs devant ajouté le nouvel USER qui vient de login
                              kill(tab->connexions[i].pidFenetre, SIGUSR1);
                          }
                        }

                      break;

      case ACCEPT_USER: fprintf(stderr,"\t\t(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);

                        //Recherche du pid lier au nom a accepter
                        Accepter = -1;
                        for(int i = 0; i<6 && Accepter == -1; i++){
                          if(strcmp(tab->connexions[i].nom, m.data1) == 0){
                            Accepter = tab->connexions[i].pidFenetre;
                          } 
                        }


                        //Recherche de l'élément de la table qui vaut le pid de l'envoyeur:
                        modifier = -1;
                        for(int i = 0; i<6 && modifier == -1; i++){
                          if(tab->connexions[i].pidFenetre == m.expediteur){
                            modifier = i;
                          } 
                        }


                        //Ajout dans la table d'acceptation de l'envoyeur, le pid du nom a accepter
                        for (int i = 0; i < 6; i++){
                          if(tab->connexions[modifier].autres[i] == 0){
                            tab->connexions[modifier].autres[i] = Accepter;
                            i=6;
                          }
                        }


                      break;

      case REFUSE_USER: fprintf(stderr,"\t\t(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);

                        //Recherche du pid lier au nom a Refuser
                        Refuser = -1;
                        for(int i = 0; i<6 && Refuser == -1; i++){
                          if(strcmp(tab->connexions[i].nom, m.data1) == 0){
                            Refuser = tab->connexions[i].pidFenetre;
                          } 
                        }


                        //Recherche de l'élément de la table qui vaut le pid de l'envoyeur:
                        modifier = -1;
                        for(int i = 0; i<6 && modifier == -1; i++){
                          if(tab->connexions[i].pidFenetre == m.expediteur){
                            modifier = i;
                          } 
                        }


                        //Ajout dans la table d'acceptation de l'envoyeur, le pid du nom a accepter
                        for (int i = 0; i < 6; i++){
                          if(tab->connexions[modifier].autres[i] == Refuser){
                            tab->connexions[modifier].autres[i] = 0;
                            i=6;
                          }
                        }
                      break;

      case SEND:      fprintf(stderr,"\t\t(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                      
                      //Recherche de l'indice dans la table de connexion
                      Exp = -1;
                      for (int i = 0; i < 6 && Exp == -1; i++){
                        if(tab->connexions[i].pidFenetre == m.expediteur){
                            Exp = i;
                        }
                      }


                      //Envoie des x messages aux user pouvant recevoir le message Si On trouve l'utilisateur dans la table de connexion
                      if(Exp != -1){
                        for(int i = 0 ; i<5 ; i++){
                          if(tab->connexions[Exp].autres[i] != 0){
                              reponse.type = tab->connexions[Exp].autres[i];
                              reponse.expediteur = 1;
                              reponse.requete = SEND;
                              strcpy(reponse.data1, tab->connexions[Exp].nom);
                              strcpy(reponse.data2, "");
                              strcpy(reponse.texte, m.texte);

                              if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                                perror("(SERVEUR) Erreur lors du SEND");
                                kill(getpid(),SIGINT); //Exit propre
                              }

                              kill(tab->connexions[Exp].autres[i], SIGUSR1);
                          }
                        }
                      }

                      break; 

      case UPDATE_PUB:fprintf(stderr,"\t\t(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);

                      //Envoie un SIGUSR2 à toutes les pages connectées:                     
                      for(int i = 0 ; i<6 ; i++){
                        if(tab->connexions[i].pidFenetre != 0){
                          kill(tab->connexions[i].pidFenetre, SIGUSR2);
                        }
                      }

                      break;

      case CONSULT:   fprintf(stderr,"\t\t(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                        // Creation du processus Consultation
                        pidConsultation = fork();

                        //Process Fils
                        if(pidConsultation == 0){
                          execl("./Consultation.app","Consultation.app",NULL);
                        }

                        //Process Père: envoie d'un msg CONSULT avec le type de la consult
                        reponse.type = pidConsultation;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = CONSULT;
                        strcpy(reponse.data1, m.data1);
                        strcpy(reponse.data2, "");
                        strcpy(reponse.texte, "");
                          
                        if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                          perror("(SERVEUR) Erreur lors de l'envoie de requete Consultation");
                          kill(getpid(),SIGINT); //Exit propre
                        }

                      break;

      case MODIF1:    fprintf(stderr,"\t\t(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                        //Recherche du nom dans la table de connexion
                        strcpy(reponse.data1, "");


                        modifier = -1;
                        for (int i=0; i < 6 && modifier == -1 ; i++){
                          if(tab->connexions[i].pidFenetre == m.expediteur){
                            strcpy(reponse.data1, tab->connexions[i].nom);
                            modifier = i;
                          }
                        }

                        if(modifier == -1){
                          // Si on ne trouve pas le pid dans la table de connexion (ça veut dire que c'est un processus de trop)
                          fprintf(stderr,"\t\t(SERVEUR %d) ERREUR MODIF1 Pid Inconnu %d\n",getpid(),m.expediteur);
                          kill(m.expediteur, SIGKILL);
                        }

                        // Creation Process Modification
                        tab->connexions[modifier].pidModification = fork();

                        //Process Fils
                        if(tab->connexions[modifier].pidModification == 0){
                          execl("./Modification.app","Modification.app",NULL);
                        }

                        // Envoie d'une requète MODIF1
                        reponse.type = tab->connexions[modifier].pidModification;
                        reponse.expediteur = m.expediteur;
                        reponse.requete = MODIF1;
                        strcpy(reponse.data2, "");
                        strcpy(reponse.texte, "");

                        if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                          perror("(SERVEUR) Erreur lors de l'envoie de requete MODIF1");
                          kill(getpid(),SIGINT);
                        }

                      break;

      case MODIF2:    fprintf(stderr,"\t\t(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                        //Recherche du nom dans la table de connexion
                        for (int i = 0; i < 6 && m.type == 1 ; i++){
                          if(tab->connexions[i].pidFenetre == m.expediteur){
                            m.type = tab->connexions[i].pidModification;
                          }
                        }
                        
                        if(m.type == 1){
                          // Si on ne trouve pas le pid dans la table de connexion (ça veut dire que c'est un processus de trop)
                          fprintf(stderr,"\t\t(SERVEUR %d) ERREUR MODIF1 Pid Inconnu %d\n",getpid(),m.expediteur);
                          kill(m.expediteur, SIGKILL);
                        }

                        if( msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                          perror("(SERVEUR)Erreur lors de l'envoie de requete MODIF2");
                          kill(getpid(),SIGINT);
                        }
                        
                      break;

      case LOGIN_ADMIN: fprintf(stderr,"\t\t(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      if(tab->pidAdmin == 0){
                          tab->pidAdmin = m.expediteur;

                          // Envoi d'ack login a ADMIN
                          reponse.type = m.expediteur;
                          reponse.expediteur = 1;
                          reponse.requete = LOGIN_ADMIN;
                          strcpy(reponse.data1, "OK");
                          strcpy(reponse.data2, "");
                          strcpy(reponse.texte, "");
                          
                          if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                              perror("(SERVEUR) Erreur lors de l'envoie de requete LOGIN_ADMIN");
                              kill(getpid(),SIGINT);
                          }
                      }
                      else{
                          // Envoi d'ack login a ADMIN
                          reponse.type = m.expediteur;
                          reponse.expediteur = 1;
                          reponse.requete = LOGIN_ADMIN;
                          strcpy(reponse.data1, "KO");
                          strcpy(reponse.data2, "");
                          strcpy(reponse.texte, "");
                          
                          if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
                              perror("(SERVEUR) Erreur lors de l'envoie de requete LOGIN_ADMIN");
                              kill(getpid(),SIGINT);
                          }
                      }

                      break;

      case LOGOUT_ADMIN:  fprintf(stderr,"\t\t(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                          if(tab->pidAdmin == m.expediteur){
                            tab->pidAdmin = 0;
                          }
                          else{
                            fprintf(stderr,"\t\t(SERVEUR %d) le Processus %d n'est pas admin\n",getpid(),m.expediteur);
                            kill(getpid(),SIGINT);
                          }
                      break;

      case NEW_USER:  fprintf(stderr,"\t\t(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                          if(tab->pidAdmin == m.expediteur){
                            //Ajout d'un tuple mdp + login
                            fprintf(stderr,"\t\t(SERVEUR %d) Ajout en BD (%s)\n",getpid(),m.data1);
                                
                            //Préparation Requete SQL ?id?
                            sprintf(requete, "INSERT INTO UNIX_FINAL (nom, motdepasse, gsm, email) VALUES ('%s','%s','---','---');", m.data1, m.data2);
                             
                            //Requete SQL
                            //Si Erreur de Transaction
                            if(mysql_query(connexion,requete) != 0)  fprintf(stderr,"(SERVEUR %d) ERREUR MSQL %d: %s\n", getpid(), mysql_errno(connexion), mysql_sqlstate(connexion));
                            else fprintf(stderr,"\t\t(SERVEUR %d) Ajout de l'utilisateur réussie\n",getpid());
                          }
                          else{
                            fprintf(stderr,"\t\t(SERVEUR %d) Tentative de transaction par process %d non accordé\n",getpid(),m.expediteur);
                          }
                          
                      break;

      case DELETE_USER: fprintf(stderr,"\t\t(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                        if(tab->pidAdmin == m.expediteur){
                          modifier = -1;
                          for(int i=0 ; i<6 && modifier == -1 ; i++){
                            if(strcmp(tab->connexions[i].nom, m.data1) == 0){
                              modifier = i;
                            }
                          }

                          if(modifier != -1){
                            fprintf(stderr,"\t\t(SERVEUR %d) USER %s est loggué, impossible de le supprimé\n",getpid(),m.data1);
                          }
                          else{
                            //Ajout d'un tuple mdp + login
                            fprintf(stderr,"\t\t(SERVEUR %d) Supression en BD (%s)\n",getpid(),m.data1);
                                 
                            //Préparation Requete SQL ?id?
                            sprintf(requete, " DELETE FROM UNIX_FINAL WHERE nom = '%s';", m.data1);
                               
                            //Requete SQL
                            //Si Erreur de Transaction
                            if(mysql_query(connexion,requete) != 0)  fprintf(stderr,"(SERVEUR %d) ERREUR MSQL %d: %s\n", getpid(), mysql_errno(connexion), mysql_sqlstate(connexion));
                            else fprintf(stderr,"\t\t(SERVEUR %d) Supression de l'utilisateur réussie\n",getpid());
                          }
                        }
                        else{
                          fprintf(stderr,"\t\t(SERVEUR %d) Tentative de transaction par process %d non accordé\n",getpid(),m.expediteur);
                        }
                      break;

      case NEW_PUB:   fprintf(stderr,"\t\t(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      if(tab->pidAdmin == m.expediteur){
                        
                        // Tentative Ouverture du fichier de publicité pour tester si elle existe
                        fd = open("publicites.dat",O_RDONLY);

                        if(fd == -1){
                          //Creation si besoin du fichier et envoi SIGUSR1
                          fd = open("publicites.dat", O_CREAT | O_EXCL, 0644);
                          if(fd == -1){
                            perror("(SERVEUR) Erreur de open publicites.dat");
                            kill(getpid(),SIGINT);
                          }
                          fprintf(stderr,"\t\t(SERVEUR %d) Envoie SIGUR1 à PUB: %d\n",getpid(),tab->pidPublicite);
                          kill(tab->pidPublicite, SIGUSR1);
                        }
                        //Si exite on ferme sans rien faire
                        close(fd);

                        //Reouverture après création:
                        fd = open("publicites.dat",O_WRONLY | O_APPEND);
                        if(fd == -1){
                          perror("(SERVEUR) Erreur de open publicites.dat");
                          kill(getpid(),SIGINT);
                        }
                        
                        pub.nbSecondes = atoi(m.data1);
                        strcpy(pub.texte, m.texte);
                        
                        if( write(fd, &pub, sizeof(PUBLICITE)) == -1){
                          perror("(SERVEUR) Erreur lors du WRITE");
                          kill(getpid(),SIGINT);
                        }
                        close(fd);
                        
                        fprintf(stderr,"\t\t(SERVEUR %d) Ajout de la pub réussie\n",getpid());
                        
                      }
                      else{
                        fprintf(stderr,"\t\t(SERVEUR %d) Tentative de transaction par process %d non accordé\n",getpid(),m.expediteur);
                      }
                      break;
    }
    fprintf(stderr,"=============================================================================\n");
    afficheTab();
  }
}

void afficheTab(){
  fprintf(stderr,"------------------------------------------------------------------------------\n\n");
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                            tab->connexions[i].nom,
                                                            tab->connexions[i].autres[0],
                                                            tab->connexions[i].autres[1],
                                                            tab->connexions[i].autres[2],
                                                            tab->connexions[i].autres[3],
                                                            tab->connexions[i].autres[4],
                                                            tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
  fprintf(stderr,"-------------------------------------------------------------------------------\n\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Handlers de signaux /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//HandlerSIGINT
void handlerSIGINT(int sig){
  fprintf(stderr,"\n(SERVEUR %d) FIN SERVEUR\n",getpid());

  for(int i = 0 ; i<6 ; i++){
    kill(tab->connexions[i].pidFenetre, SIGINT);
  }

  kill(tab->pidPublicite, SIGKILL);

  kill(tab->pidAdmin, SIGINT);

  semctl(idSem, 0, IPC_RMID);
  shmctl(idShm, IPC_RMID, NULL);
  msgctl(idQ, IPC_RMID, NULL);

  mysql_close(connexion);

  exit(0);
}


//HandlerSIGCHLD
void handlerSIGCHLD(int sig){
  fprintf(stderr,"(SERVEUR) - SIGSHLD\n");
  int pid;
  pid = wait(NULL);
  if(pid == -1){
    perror("(SERVEUR) erreur wait()");
    kill(getpid(), SIGINT); //Exit Propre
    return;
  }

  for (int i = 0; i < 6 ; i++){
    if(tab->connexions[i].pidModification == pid){
      tab->connexions[i].pidModification = 0;
      i = 10;
    }
  }
  fprintf(stderr,"(SERVEUR %d)Supression du fils zombi %d\n",getpid(), pid);
  return;
}
