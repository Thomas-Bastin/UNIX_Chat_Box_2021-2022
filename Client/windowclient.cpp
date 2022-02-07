//Basique Prof
#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "./dialogmodification/dialogmodification.h"
#include <unistd.h>

//IPC
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

//Errno
#include <errno.h>

//Wait
#include<sys/wait.h>

//Signaux
#include <signal.h>

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm, fd;
#define TIME_OUT 120
int timeOut = TIME_OUT;
int CompteurUser = 0;
char *ShmString;

//Handler de Signaux
void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);
void handlerSIGINT(int sig);
void handlerSIGALRM(int sig);

//Fonction Perso
void Accepter(const char*);
void Refuser(const char*);
void ResetSigAlarm(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    logoutOK();

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
    idQ = msgget(CLE, 0);

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    idShm = shmget(CLE, sizeof(char[200]), 0);

    // Attachement à la mémoire partagée en lecture seulement
    ShmString = (char *)shmat(idShm, NULL, SHM_RDONLY);
    if(ShmString == (char *) -1){
      perror("(CLIENT) Erreur d'attachement mémoire");
      exit(1);
    }

    // Armement des signaux
    struct sigaction Action;
    
    // SIGUSR1
    Action.sa_handler = handlerSIGUSR1;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &Action,NULL) == -1){
      fprintf(stderr,"(CLIENT %d) Erreur de Sigaction\n",getpid());
    }

    // SIGUSR2
    Action.sa_handler = handlerSIGUSR2;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR2, &Action,NULL) == -1){
      fprintf(stderr,"(CLIENT %d) Erreur de Sigaction\n",getpid());
    }
    
    // SIGINT
    Action.sa_handler = handlerSIGINT;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = SA_RESTART;
    if(sigaction(SIGINT, &Action,NULL) == -1){
      fprintf(stderr,"(CLIENT %d) Erreur de Sigaction\n",getpid());
    }

    // SIGALRM
    Action.sa_handler = handlerSIGALRM;
    sigemptyset(&Action.sa_mask);
    Action.sa_flags = SA_RESTART;
    if(sigaction(SIGALRM, &Action,NULL) == -1){
      fprintf(stderr,"(CLIENT %d) Erreur de Sigaction\n",getpid());
    }
    

    // Envoi d'une requete de connexion au serveur
    MESSAGE reponse;

    reponse.type = 1;
    reponse.expediteur = getpid();
    reponse.requete = CONNECT;
    strcpy(reponse.data1, "");
    strcpy(reponse.data2, "");
    strcpy(reponse.texte, "");
    
    if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
        perror("(CLIENT) Erreur lors de l'envoie de requete CONNECT");
        exit(1);
    }
}

WindowClient::~WindowClient(){
  delete ui;
}

//Surcharge de CloseEvent (la croix de l'interface QT)
void WindowClient::closeEvent(QCloseEvent *event)
{
  //Au lieu de simplement fermé la fenetre, l'event est ignoré, et on ferme en utilisant le bouton quitté.
  event->ignore();
  on_pushButtonQuitter_clicked();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text){
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom(){
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text){
  if (strlen(Text) == 0 ){
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse(){
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text){
  if (strlen(Text) == 0 ){
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb){
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text){
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 ){
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer(){
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text){
  if (strlen(Text) == 0 ){
    switch(i){
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i){
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i){
  QLineEdit *tmp;
  switch(i){
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message){
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve){
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve){
      switch(i){
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text){
  if (strlen(Text) == 0 ){
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements(){
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text){
  if (strlen(Text) == 0 ){
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text){
  if (strlen(Text) == 0 ){
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b){
  QCheckBox *tmp;
  switch(i){
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK(){
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK(){

  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  
  for (int i=1 ; i<=5 ; i++){
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
  ResetSigAlarm();
  
  //Préparation Requète
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = LOGIN;
  strcpy(reponse.data1, getNom());
  strcpy(reponse.data2, getMotDePasse());
  strcpy(reponse.texte, "");
    
  //Envoie Requète Login
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete LOGIN");
    exit(1);
  }
}

void WindowClient::on_pushButtonLogout_clicked()
{
  ResetSigAlarm();

  //Préparation Requète
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = LOGOUT;
  strcpy(reponse.data1, "");
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  //Envoie Requète Logout
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete LOGOUT");
    exit(1);
  }

  logoutOK();
}

void WindowClient::on_pushButtonQuitter_clicked()
{
  on_pushButtonLogout_clicked();
  
  MESSAGE reponse;
  
  //Envoie un message de déconnection au serveur
  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = DECONNECT;
  strcpy(reponse.data1, "");
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete DECONNECT");
    exit(1);
  }

  exit(0);
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
  ResetSigAlarm();
  alarm(1);

  if(strcmp(getAEnvoyer(), "") == 0) return;

  MESSAGE reponse;

  //Envoie un message SEND au serveur
  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = SEND;
  strcpy(reponse.data1, "");
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, getAEnvoyer());
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete SEND");
    exit(1);
  }

  //Envoie du Message dans la conv en local:
  ajouteMessage(getNom(),getAEnvoyer());

  //reset de la box d'entrée de msg
  setAEnvoyer("");
}

void WindowClient::on_pushButtonConsulter_clicked()
{
  ResetSigAlarm();
  alarm(1);

  if(strcmp(getNomRenseignements(), "") == 0) return;

  //Envoie un message CONSULT au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = CONSULT;
  strcpy(reponse.data1, getNomRenseignements());
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete CONSULT");
    exit(1);
  }

  setGsm("...En Attente...");
  setEmail("...En Attente...");

  return;
}

void WindowClient::on_pushButtonModifier_clicked()
{
  ResetSigAlarm();
  alarm(1);
  
  // Envoi d'une requete MODIF1 au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = MODIF1;
  strcpy(reponse.data1, "");
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete MODIF1");
    exit(1);
  }

  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  MESSAGE m;

  fprintf(stderr,"(CLIENT %d) Attente d'une requete...\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1){
    perror("(CLIENT) Erreur de msgrcv\n");
    exit(1);
  }


  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  //Prep Requète MODIF2
  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = MODIF2;

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),m.data1,m.data2,m.texte);
  dialogue.exec();

  strcpy(reponse.data1, dialogue.getMotDePasse()); 
  strcpy(reponse.data2, dialogue.getGsm());
  strcpy(reponse.texte, dialogue.getEmail());
  

  //Push OK
  // Envoi des données modifiées au serveur
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete MODIF2");
    exit(1);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
  ResetSigAlarm();
  alarm(1);
    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        Accepter(getPersonneConnectee(1));
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        Refuser(getPersonneConnectee(1));
    }
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
  ResetSigAlarm();
  alarm(1);
    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        Accepter(getPersonneConnectee(2));
    }
    else
    {
        ui->checkBox2->setText("Refusé");
        Refuser(getPersonneConnectee(2));
    }
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
  ResetSigAlarm();
  alarm(1);
    if (checked)
    {
        ui->checkBox3->setText("Accepté");
        Accepter(getPersonneConnectee(3));
    }
    else
    {
        ui->checkBox3->setText("Refusé");
        Refuser(getPersonneConnectee(3));
    }
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
  ResetSigAlarm();
  alarm(1);
    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        Accepter(getPersonneConnectee(4));
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        Refuser(getPersonneConnectee(4));
    }
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
  ResetSigAlarm();
  alarm(1);
    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        Accepter(getPersonneConnectee(5));
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        Refuser(getPersonneConnectee(5));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Handlers de signaux /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    int ajouter, retirer;
    int ret;

    fprintf(stderr,"(CLIENT %d - SIGUSR1 %d) Attente d'une requete...\n",getpid(),sig);
    int i = 0;
    while(i == 0){

      ret = msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0);
      if(ret < (int)sizeof(MESSAGE) - (int)sizeof(long)) break;
      if(ret == 0) break;
      if(ret == -1) exit(5);


      switch(m.requete){
        case LOGIN :
                    if(strcmp(m.data1,"OK") == 0){
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      QMessageBox::information(w,"Login...","Login Ok ! Bienvenue...");

                      timeOut = TIME_OUT;
                      alarm(1);
                    }
                    else{
                      if(strcmp(m.data2, "MDP") == 0)   QMessageBox::critical(w,"Login...","Erreur Mot De Passe Incorrect...");
                      
                      if(strcmp(m.data2,"USER") == 0)   QMessageBox::critical(w,"Login...","Erreur Nom d'utilisateur inconnu...");
                      
                      if(strcmp(m.data2, "DUP") == 0)   QMessageBox::critical(w,"Login...","Erreur Utilisateur déjà connecté...");
                      i = 1;
                    }    
                    break;

        case ADD_USER :
                      fprintf(stderr,"(CLIENT %d) ADD_USER: %s\n",getpid(),m.data1);
                      ajouter = -1;
                      for (int i = 1; i < 6 && ajouter == -1; ++i){
                        if(strcmp(w->getPersonneConnectee(i),"") == 0){
                          ajouter = i;
                        }
                      }
                      if(ajouter != -1){
                          w->setPersonneConnectee(ajouter,m.data1);
                          w->ajouteMessage(m.data1,"est Connecter...");
                          w->setCheckbox(ajouter,false);
                      }
                    

                    break;

        case REMOVE_USER :
                    fprintf(stderr,"(CLIENT %d) REMOVE_USER: %s\n",getpid(),m.data1);
                    retirer = -1;
                    for (int i = 1; i < 6 && retirer == -1; ++i){
                      if(strcmp(w->getPersonneConnectee(i),m.data1) == 0){
                        retirer = i;
                      }
                    }
                    if(retirer != -1){
                        if(strcmp(m.data1,"") != 0){
                          w->ajouteMessage(m.data1,"est Deconnecter...");
                        }
                        //On enlève l'utilisateur
                        w->setPersonneConnectee(retirer,"");
                        w->setCheckbox(retirer,false);
                    }
                    break;

        case SEND :
                    fprintf(stderr,"(CLIENT %d) Reception MSG From %s\n",getpid(),m.data1);
                    //ajout du Message dans la conv en local:
                    w->ajouteMessage(m.data1,m.texte);
                    break;

        case CONSULT :
                    if(strcmp(m.data1,"OK") == 0){
                      w->setGsm(m.data2);
                      w->setEmail(m.texte);
                    }

                    if(strcmp(m.data1,"KO") == 0){
           
                      w->setGsm("NON TROUVE");
                      w->setEmail("NON TROUVE");
                    }
                    
                    break;
      }
    }
}

void handlerSIGUSR2(int sig){
  w->setPublicite(ShmString);
}


void handlerSIGINT(int sig){
  fprintf(stderr,"Réception SIGINT(%d)\n",sig);
  //Si Sigint, on quitte
  w->on_pushButtonQuitter_clicked();
}

void handlerSIGALRM(int sig){
  //si timeout == 0 on logout
  if(timeOut == 0){
    w->on_pushButtonLogout_clicked();
    return;
  }
  //Si non on décrémente et on relance le signal dans 1 sec.
  w->setTimeOut(--timeOut);
  alarm(1);
}



////////////////////////////////////////////////////////////////////////////////////////
///////////////////////          FONCTION PERSO            /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

//Accepte un User pour l'envoie de message
void Accepter(const char* Nom){
  if(strcmp(Nom,"") == 0) return;

  MESSAGE reponse;

  //Envoie un message ACCEPT_USER au serveur
  fprintf(stderr,"(CLIENT %d) ACCEPT_USER: %s\n",getpid(), Nom);
  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = ACCEPT_USER;
  strcpy(reponse.data1, Nom);
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete ACCEPT_USER");
    exit(1);
  }
}

//Refuse un User pour l'envoie de message
void Refuser(const char* Nom){
  if(strcmp(Nom,"") == 0) return;
  MESSAGE reponse;

  //Envoie un message REFUSE_USER au serveur
  fprintf(stderr,"(CLIENT %d) REFUSE_USER: %s\n",getpid(), Nom);
  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = REFUSE_USER;
  strcpy(reponse.data1, Nom);
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(CLIENT) Erreur lors de l'envoie de requete REFUSE_USER");
    exit(1);
  }
}

//ResetSigAlarm
void ResetSigAlarm(void){
  alarm(0);
  timeOut = TIME_OUT;
  w->setTimeOut(timeOut);
}
