#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include "Constante&Struct.h"

int dSFile;
int dS;

void * sendFile(void *fp) {
  char buffer[MEMOIRE_CACHE];
  int taillebuffer=MEMOIRE_CACHE;
  while(MEMOIRE_CACHE==taillebuffer){// Quand taillebuffer n'est plus égal à MEMOIRE_CACHE, on arrive a la fin du fichier
    taillebuffer=fread(buffer, 1, MEMOIRE_CACHE, fp); // fread retourne la taille du fichier lu
    send(dSFile, buffer, sizeof(char)*taillebuffer, 0);
  }
  fclose(fp);
  printf("Envoi terminé !");
  pthread_exit(NULL); 
}
void * recvFile(void *filenameToRcv) {
  int sendfileName = send(dSFile,filenameToRcv, CARACTERES_MAX, 0);
  char chemin[60] = "fichiers/";
  strcat(chemin,filenameToRcv);
  chemin[strlen(chemin)-1]='\0';
  long int length;
  recv(dSFile, &length,sizeof(long int), 0);
  if (length == -1) {//Le serveur envoie -1 si le nom de fichier n'existe pas
    printf("Le fichier n'existe pas sur le serveur\n");
  } else {
    FILE* file;
    file = fopen(chemin,"wb");
    long int position=0;
    char buffer[MEMOIRE_CACHE];
    int taillebuffer=MEMOIRE_CACHE;
    while(length > position + MEMOIRE_CACHE){
      recv(dSFile, buffer, sizeof(char)*MEMOIRE_CACHE, 0);
      position+=fwrite(buffer, 1, MEMOIRE_CACHE, file);
    }
    recv(dSFile, buffer, sizeof(char)*MEMOIRE_CACHE, 0);
    fwrite(buffer, 1, length - position, file);
    printf("Fin\n");
    fclose(file);
  }
  pthread_exit(NULL); 
  
}
void * readingMessages(void *dS) {
    int dSbis = (int) dS;
    int i = 1;
    Message n;
    while (i == 1) {
        if (i != -1) { // Afin de sortir de la boucle et terminer le programme en cas de fin de transmission (nous serions en attente de réception sans cette condition)
            int recu = recv(dSbis, &n, sizeof(Message), 0) ;
            if (recu>0){
                printf("%s --> %s\n", n.username, n.msg) ;
            }
            if (strcmp(n.msg, "fin") == 0){
                printf("Fin de la transmission...\n");
                i = -1;
                close(dSbis);
            }
            if (recu==-1){
                printf("Serveur fermé\n") ;
                i = -1;
                close(dSbis) ;
            }
        }
    }
    pthread_exit(NULL); 
}

int main(int argc, char *argv[]) {

  //Création et connexion à la socket principale
  dS = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;
  int connected;
  int connecte = connect(dS, (struct sockaddr *) &aS, lgA);

  //Création et connexion a la socket des fichiers
  dSFile = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in aSFile;
  aSFile.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aSFile.sin_addr)) ;
  aSFile.sin_port = htons(atoi(argv[2]) + 1);
  socklen_t lgAFile = sizeof(struct sockaddr_in) ;
  int connectFile = connect(dSFile, (struct sockaddr *) &aSFile, lgAFile);

  //Retour à l'utilisateur sur l'état de la connexion
  if (connecte == 0){
      printf("Connexion en cours...\n");
      int recDestExist = recv(dS, &connected, sizeof(int), 0);
      printf("Connexion réussie\n");
  }else {
      printf("Connexion échouée, merci de relancer le programme avec la bonne adresse IP et le bon port.");
      return 0;
  }
  

  //Initialisations
  int i =1;
  char m[CARACTERES_MAX]; //message privée envoyé au destinataire
  char username[25]; //pseudo de l'utilisateur
  char destinataire[25]; //destinataire du message privé
  char commande[25]; //commande que l'utilisateur envoie
  char filenameToSend[60]; //nom du fichier à envoyer
  int envoiPseudo; //envoie du pseudo au serveur 
  int pseudoExistePas = 0; // 1 si le pseudo n'est pas déjà pris, 0 si il est déjà utilisé sur le serveur
  int recPseudoExistePas; //reçu de la validation du pseudo
  char nomSalon[CARACTERES_MAX]; //nom du salon à rejoindre ou à créer
  char descriptionSalon[CARACTERES_MAX]; //description du salon à créer
  Salon nouveauSalon; //nouveau salon à créer
  Message msgToSend; //message à envoyer à l'utilisateur en privée
  int nb_salons; //nombre de salons présent
  Salon* Liste_Salon=calloc(1, sizeof(Salon)); //crée un tableau de salons
  int isAdmin = 0; //0 si l'utilisateur n'est pas admin et 1 s'il est admin


  //Gestion du nom d'utilisateur
  while (!pseudoExistePas){ //tant que le pseudo n'existe oas faire
      printf("Entrer un nom d'utilisateur :\n");
      fgets(username,CARACTERES_MAX,stdin); //demande à l'utilisateur un choix de pseudo
      username[strlen(username)-1]='\0'; // Pour enlever le retour à la ligne
      envoiPseudo = send(dS, username, 25*sizeof(char) , 0) ; //envoie du pseudo au serveur
      recPseudoExistePas = recv(dS, &pseudoExistePas, sizeof(int), 0); //reçu de la validation ou non du pseudo par le serveur
      if (!pseudoExistePas) { //si le pseudo n'est pas validé
          printf("Le nom d'utilisateur est déjà utilisé, merci d'en entrer un autre.\n");
      }
  }

  if (strcmp(username, "admin") == 0){
    isAdmin = 1;
  }

  //Création des threads et lancement du thread de réception de messages. L'envoi de message se fait sur le thread principal.
  pthread_t thread;
  pthread_t threadFileReception;
  pthread_t threadFileSend;
  pthread_create(&thread,NULL, readingMessages,  (void*) dS);
  
  //Boucle d'envoie des commandes'
  while (i==1){

      
      printf("Entrer une commande ou un message si vous êtes dans un salon :\n");
      fgets(commande,CARACTERES_MAX,stdin); //demande à l'utilisateur d'écrire une commande
      commande[strlen(commande)-1]='\0'; // Pour enlever le retour à la ligne
      send(dS, commande, strlen(commande)+1 , 0) ; //envoie de la commande au serveur
      
      //Gestion de l'envoi de fichier
      if (strcmp(commande,"/fileSend")==0){
        printf("Entrez le nom du fichier \n");
        fgets(filenameToSend,40,stdin); //demande à l'utilisateur d'entrer le nom du fichier
        FILE *fp;
        char chemin[60] = "fichiers/"; //tout les fichiers sont présent dans le dossier fichiers
        strcat(chemin,filenameToSend); //concaténation du chemin et du nom du fichier
        chemin[strlen(chemin)-1]='\0'; // Pour enlever le retour à la ligne
        printf("%s\n",chemin);
        fp = fopen(chemin, "rb"); //ouvre le fichier choisi
        if (fp == NULL){ //si le fichier est vide
          printf("%s\n",strerror( errno ));//Print une erreur à l'utilisateur
          char* err="erreur";
          send(dS,err,strlen(err)+1,0);//envoi d'erreur au serveur
        }
        else{ //si le fichier n'est pas vide
          
          int sendChemin = send(dS,chemin, CARACTERES_MAX, 0); //envoie du chemin au serveur
          if (sendChemin == 0){ //si erreur d'envoie
            printf("Send chemin error \n");
            exit(0);
          } 
          //Récupération de la taille du fichier
          long int lenght; 
          fseek(fp,0,SEEK_END); // Place le curseur a la fin du fichier
          lenght=ftell(fp);
          fseek(fp,0,SEEK_SET);
          printf("%ld\n",lenght);
          send(dS,&lenght,sizeof(long int),0); //envoie la taille du fichier au serveur
          printf("Envoi du fichier...\n");
          pthread_create(&threadFileReception,NULL, sendFile,  (void*) fp);
        }
      }

      //Gestion de la réception de fichiers
      else if (strcmp(commande,"/fileRecv")==0) {
        
        char liste_fichiers[50][50]; //liste des fichiers disponible
        recv(dSFile, liste_fichiers, 50*50*sizeof(char), 0); 
        printf("Liste des fichiers : \n");       
        int i=0;
        while ((i<50) && (strlen(liste_fichiers[i])>0)){ //On arrête la boucle si il n'y a plus de noms de fichiers dans le tableau
          printf("%s \n",liste_fichiers[i]);
          i++;
        }
        char filenameToRcv[50];
        fgets(filenameToRcv,50,stdin);
        
        pthread_create(&threadFileReception,NULL, recvFile, (void*) filenameToRcv);
      }
      else if (strcmp(commande,"/joinSalon")==0) { 
        recv(dSFile, &nb_salons,sizeof(int),0); //reçoit du serveur le nombre de salons disponibles
        Liste_Salon = realloc(Liste_Salon,sizeof(Salon)*nb_salons);//réallocation du tableau pour gérer s'il quitte un salon puis en rejoint un
        recv(dSFile, Liste_Salon, nb_salons*sizeof(Salon),0); //reçoit la liste des salons disponibles
        printf("Il y a %d salons existants. \n Veuillez choisir un salon parmis les suivants : \n", nb_salons);
        for(int i = 0; i<nb_salons; i++){ //affichage des salons disponibles
          printf("Salon %d  Nom : %s  Slot : %d/%d \n Description : %s \n",i+1, Liste_Salon[i].nomSalon, Liste_Salon[i].personnesActuelles,Liste_Salon[i].personnesMax, Liste_Salon[i].descriptionSalon);
        }
        char salonChoisi[25]; //salon choisi par l'utilisateur
        fgets(salonChoisi,25,stdin);
        salonChoisi[strlen(salonChoisi)-1]='\0'; // Pour enlever le retour à la ligne
        send(dSFile,salonChoisi,25*sizeof(char),0); //envoie le nom du salon choisi par l'utilisateur
      } 
      else if (strcmp(commande,"/createSalon")==0) {
      
        if (isAdmin == TRUE) { //check si l'user est un admin avant de lui donner la permission de crééer le salon
        
          printf("Création de salon\nMerci d'entrer le nom du salon que vous souhaitez créer: ");
          fgets(nomSalon,CARACTERES_MAX,stdin); //demande le nom du salon à créeer
          nomSalon[strlen(nomSalon)-1]='\0';
          strcpy(nouveauSalon.nomSalon, nomSalon);
          printf("Entrer la description du salon: \n");
          fgets(descriptionSalon,CARACTERES_MAX,stdin); //demande la description du salon à créer
          descriptionSalon[strlen(descriptionSalon)-1]='\0';
          strcpy(nouveauSalon.descriptionSalon, descriptionSalon);
          printf("Entrer le nombre maximum de personnes du salon: \n");
          scanf("%d",&(nouveauSalon.personnesMax)); //demande le nombre de personnes maximum du salon à créer


          int envoi = send(dS, &nouveauSalon, sizeof(Salon), 0) ; //envoie les infos du salons à créer
        
        } else {
          printf("Vous n'avez pas les droits de création de salons.");
        }
          
        
        
      }
      else if (strcmp(commande,"/modifSalon")==0) {
      
        if (isAdmin == TRUE) { //check si l'user est un admin avant de lui donner la permission de modifier le salon
        
          printf("Modification de salon\nMerci d'entrer le nom du salon que vous souhaitez modifier: ");
          fgets(nomSalon,CARACTERES_MAX,stdin); //demande le nom du salon à modifier
          nomSalon[strlen(nomSalon)-1]='\0';
          send(dS, nomSalon, 1000*sizeof(char), 0) ;


          printf("Modification de salon\nMerci d'entrer le nouveau nom du salon : ");
          fgets(nomSalon,CARACTERES_MAX,stdin); //demande le nom du salon à modifier
          nomSalon[strlen(nomSalon)-1]='\0';
          strcpy(nouveauSalon.nomSalon, nomSalon);
          printf("Entrer la nouvelle description du salon: \n");
          fgets(descriptionSalon,CARACTERES_MAX,stdin); //demande la description du salon à modifier
          descriptionSalon[strlen(descriptionSalon)-1]='\0';
          strcpy(nouveauSalon.descriptionSalon, descriptionSalon);
          printf("Entrer le nouveau nombre maximum de personnes du salon: \n");
          scanf("%d",&(nouveauSalon.personnesMax)); //demande le nombre de personnes maximum du salon à modifier


          int envoi = send(dS, &nouveauSalon, sizeof(Salon), 0) ; //envoie les infos du salons à modifier
        
        } else {
          printf("Vous n'avez pas les droits de modification de salons.");
        }
      }
      else if (strcmp(commande,"/deleteSalon")==0) {
      
        if (isAdmin == TRUE) { //check si l'user est un admin avant de lui donner la permission de supprimmer le salon
        
          printf("Supression de salon\nMerci d'entrer le nom du salon que vous souhaitez supprimmer: ");
          fgets(nomSalon,CARACTERES_MAX,stdin); //demande le nom du salon à supprimmer
          nomSalon[strlen(nomSalon)-1]='\0';
          strcpy(nouveauSalon.nomSalon, nomSalon);
          int envoi = send(dS, &nouveauSalon, sizeof(Salon), 0) ;//envoie le salon à supprimmer
        
        } else {
          printf("Vous n'avez pas les droits de supression de salons.");
        }
          
        
        
      }
      else if(strcmp(commande,"/fin")==0){// Si l'utilisateur entre /fin, fermeture de la connexion
        printf("Fin de la transmission...\n");
        i = -1; //Afin de sortir de la boucle while
        close(dS);
      }
      


      // Si ce n'est pas une commande qui est entrée mais un destinataire
      else if (strcmp(commande,"/private")==0){
        printf("Entrez le destinataire :");
        fgets(destinataire,25*sizeof(char),stdin);
        destinataire[strlen(destinataire)-1]='\0'; // Pour enlever le retour à la ligne
        send(dS, destinataire, 25*sizeof(char), 0) ;

        //Envoi de message
        printf("Écrire un message :\n");
        fgets(m,CARACTERES_MAX,stdin);
        m[strlen(m)-1]='\0'; // Pour remplacer le retour a la ligne par le caractère de fin de message
        strcpy(msgToSend.msg, m);
        strcpy(msgToSend.username, destinataire);
        int envoi = send(dS, m, CARACTERES_MAX, 0) ;
        if (envoi==-1){// Si il y a une erreur
            printf("Serveur fermé\n") ;
            close(dS) ;
            i=-1;
        }
      }
  }
}