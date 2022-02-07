#include "windowadmin.h"
#include "ui_windowadmin.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

//Signaux
#include <signal.h>

extern int idQ;
extern WindowAdmin *w; //Pour gérer handlerSIGINT

//Signaux
void handlerSIGINT(int sig);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowAdmin::WindowAdmin(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowAdmin)
{
  fprintf(stderr,"(ADMIN %d) Armememnt SIGINT\n",getpid());
  // Armement des signaux
  struct sigaction Action;

  // SIGINT
  Action.sa_handler = handlerSIGINT;
  sigemptyset(&Action.sa_mask);
  Action.sa_flags = 0;
  if(sigaction(SIGINT, &Action,NULL) == -1){
    fprintf(stderr,"(SERVEUR %d) Erreur de Sigaction\n",getpid());
  }

  ui->setupUi(this);
}

WindowAdmin::~WindowAdmin()
{ 
  delete ui;
}

//Surcharge de CloseEvent (la croix de l'interface QT)
void WindowAdmin::closeEvent(QCloseEvent *event)
{
  //Au lieu de simplement fermé la fenetre, l'event est ignoré, et on ferme en utilisant le bouton quitté.
  event->ignore();
  on_pushButtonQuitter_clicked();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setTexte(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditTexte->clear();
    return;
  }
  ui->lineEditTexte->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getTexte()
{
  strcpy(texte,ui->lineEditTexte->text().toStdString().c_str());
  return texte;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNbSecondes(int n)
{
  char Text[10];
  sprintf(Text,"%d",n);
  ui->lineEditNbSecondes->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowAdmin::getNbSecondes()
{
  char tmp[10];
  strcpy(tmp,ui->lineEditNbSecondes->text().toStdString().c_str());
  return atoi(tmp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::on_pushButtonAjouterUtilisateur_clicked()
{
  if(strcmp(getNom(), "") == 0 || strcmp(getMotDePasse(), "") == 0){
    QMessageBox::critical(w,"Ajout Utilisateur","Le Nom et le MotDePasse ne peuvent pas être vide...");
    return;
  }
  // A -> S    Data 1 = login          Data2 = mot de passe
  // Envoi d'une requete de NEW_USER au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = NEW_USER;
  strcpy(reponse.data1, getNom());
  strcpy(reponse.data2, getMotDePasse());
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(ADMIN) Erreur lors de l'envoie de requete NEW_USER");
    exit(1);
  }
}

void WindowAdmin::on_pushButtonSupprimerUtilisateur_clicked()
{
  if(strcmp(getNom(), "") == 0){
    QMessageBox::critical(w,"Supression Utilisateur","Le Nom ne peut pas être vide...");
    return;
  }
  // A -> S    data1 = login
  // Envoi d'une requete de DELETE_USER au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = DELETE_USER;
  strcpy(reponse.data1, getNom());
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(ADMIN) Erreur lors de l'envoie de requete DELETE_USER");
    exit(1);
  }
}

void WindowAdmin::on_pushButtonAjouterPublicite_clicked()
{
  if(strcmp(getTexte(), "") == 0){
    QMessageBox::critical(w,"Ajout Publicité","Le texte de la publicité ne peut pas être Vide...");
    return;
  }
  // A -> S   data1 = nbSecondes                   Texte = texte de la pub
  // Envoi d'une requete de NEW_PUB au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = NEW_PUB;
  sprintf(reponse.data1,"%d",getNbSecondes());
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, getTexte());
    
  if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(ADMIN) Erreur lors de l'envoie de requete NEW_PUB");
    exit(1);
  }
}

void WindowAdmin::on_pushButtonQuitter_clicked()
{
  // Envoi d'une requete de logout au serveur
  MESSAGE reponse;

  reponse.type = 1;
  reponse.expediteur = getpid();
  reponse.requete = LOGOUT_ADMIN;
  strcpy(reponse.data1, "");
  strcpy(reponse.data2, "");
  strcpy(reponse.texte, "");
    
  if( msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1){
    perror("(ADMIN) Erreur lors de l'envoie de requete LOGOUT_ADMIN");
    exit(1);
  }

  exit(0);
}


/////////////////////////////////////////////////////////////////////////////
////////////////////////////  SIGNAUX  //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//SIGINT
void handlerSIGINT(int sig){
  fprintf(stderr,"\n(ADMIN %d) Passage SIGINT(%d)\n", getpid(), sig);
  w->WindowAdmin::on_pushButtonQuitter_clicked();
}