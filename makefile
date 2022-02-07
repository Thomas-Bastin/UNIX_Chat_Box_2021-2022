#MakeFile
#Silent pour echo:
.SILENT:

#MakeAllFile
All:	Admin Client CreationBD BidonFichierPub Serveur Modification Publicite Consultation


#Variable pour création file 
Fichier = ./Fichier/
Client = ./Client/
Admin = ./Admin/
dialog = ./Client/dialogmodification/
obj = ./obj/
Serveur = ./Serveur/

#Administrateur
Admin:	$(obj)mainAdmin.o $(obj)windowadmin.o $(obj)moc_windowadmin.o
		echo Creation Administrateur
		g++ $(obj)mainAdmin.o $(obj)windowadmin.o $(obj)moc_windowadmin.o  -o Administrateur.app	/usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread

$(obj)windowadmin.o:	$(Admin)windowadmin.cpp	$(Admin)windowadmin.h $(Admin)windowadmin.h protocole.h
				echo Creation windowadmin.o
				g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)windowadmin.o $(Admin)windowadmin.cpp

$(obj)mainAdmin.o:	$(Admin)mainAdmin.cpp $(Admin)windowadmin.h protocole.h
				echo Creation mainAdmin.o
				g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)mainAdmin.o $(Admin)mainAdmin.cpp

$(obj)moc_windowadmin.o:	$(Admin)moc_windowadmin.cpp $(Admin)windowadmin.h protocole.h
					echo Creation moc_windowadmin.o
					g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)moc_windowadmin.o $(Admin)moc_windowadmin.cpp


#Client
Client:	$(obj)dialogmodification.o $(obj)mainClient.o $(obj)windowclient.o $(obj)moc_dialogmodification.o $(obj)moc_windowclient.o
		echo Creation Clients
		g++  -o Client.app $(obj)dialogmodification.o $(obj)mainClient.o $(obj)windowclient.o $(obj)moc_dialogmodification.o $(obj)moc_windowclient.o   /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread

$(obj)mainClient.o:	$(Client)mainClient.cpp $(Client)windowclient.h
				echo Creation mainClient.o
				g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)mainClient.o $(Client)mainClient.cpp

$(obj)windowclient.o:		$(Client)windowclient.cpp $(Client)windowclient.h $(Client)ui_windowclient.h $(dialog)dialogmodification.h protocole.h
					echo Creation windowclient.o
					g++ -c -pipe -g -std=gnu++11 -Wall -Wno-unused-parameter -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)windowclient.o $(Client)windowclient.cpp

$(obj)moc_windowclient.o:		$(Client)moc_windowclient.cpp $(Client)windowclient.h
						echo Creation moc_windowclient.o
						g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)moc_windowclient.o $(Client)moc_windowclient.cpp


#dialogmodification
$(obj)dialogmodification.o:	$(dialog)dialogmodification.cpp $(dialog)dialogmodification.h $(dialog)ui_dialogmodification.h
						echo Creation dialogmodification.o
						g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)dialogmodification.o $(dialog)dialogmodification.cpp

$(obj)moc_dialogmodification.o:	$(dialog)moc_dialogmodification.cpp $(dialog)dialogmodification.h
							echo Creation moc_dialogmodification.o
							g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o $(obj)moc_dialogmodification.o $(dialog)moc_dialogmodification.cpp



#Gestion Fichier
CreationBD:	$(Fichier)CreationBD.cpp
			echo CreationBD
			g++ -o CreationBD.app $(Fichier)CreationBD.cpp -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

BidonFichierPub:	$(Fichier)BidonFichierPub.cpp
					echo BidonFichierPub
					g++ -o BidonFichierPub.app $(Fichier)BidonFichierPub.cpp



#SubProcess
Serveur:	$(Serveur)Serveur.cpp protocole.h
			echo Serveur
			g++ $(Serveur)Serveur.cpp -o Serveur.app -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

Modification:	$(Serveur)Modification.cpp protocole.h
				echo Modification
				g++ $(Serveur)Modification.cpp -o Modification.app -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


Publicite:	$(Serveur)Publicite.cpp protocole.h
			echo Publicite
			g++ $(Serveur)Publicite.cpp -o Publicite.app


Consultation:	$(Serveur)Consultation.cpp protocole.h
				echo Consultation
				g++ $(Serveur)Consultation.cpp -o Consultation.app -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

clean:
		rm $(obj)*.o
		clear
		rm Administrateur.app
		clear
		rm Client.app
		clear
		rm BidonFichierPub.app
		clear
		rm Consultation.app
		clear
		rm CreationBD.app
		clear
		rm Modification.app
		clear
		rm Publicite.app
		clear
		rm Serveur.app
		clear
