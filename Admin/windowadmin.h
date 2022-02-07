#ifndef WINDOWADMIN_H
#define WINDOWADMIN_H

#include <QMessageBox>
#include <QMainWindow>
#include <QCloseEvent>
#include "../protocole.h"

QT_BEGIN_NAMESPACE
namespace Ui { class WindowAdmin; }
QT_END_NAMESPACE

class WindowAdmin : public QMainWindow
{
    Q_OBJECT

public:
    WindowAdmin(QWidget *parent = nullptr);
    ~WindowAdmin();

    void setNom(const char* Text);
    const char* getNom();
    void setMotDePasse(const char* Text);
    const char* getMotDePasse();
    void setTexte(const char* Text);
    const char* getTexte();
    void setNbSecondes(int n);
    int getNbSecondes();

private slots:
    void on_pushButtonAjouterUtilisateur_clicked();
    void on_pushButtonSupprimerUtilisateur_clicked();
    void on_pushButtonAjouterPublicite_clicked();

public slots:
    void on_pushButtonQuitter_clicked();

private:
    //Surcharge de CloseEvent (la croix de l'interface QT)
    void closeEvent(QCloseEvent *event);

    Ui::WindowAdmin *ui;

    char nom[20];
    char motDePasse[20];
    char texte[200];
    int  nbSecondes;
};
#endif // WINDOWADMIN_H
