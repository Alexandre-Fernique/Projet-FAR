#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <pthread.h> 
#include <semaphore.h>
#include <dirent.h>

#define CARACTERES_MAX 1000
#define MEMOIRE_CACHE 1024 //octet
#define NB_CONNEXION_MAX 3
#define NB_SALON_MAX 5
#define PERSONNE_MAX 10
#define TRUE 1
#define FALSE 0
#define ERREUR -1

typedef struct Message{
  char username[25];//username expediteur
  char msg[CARACTERES_MAX];//message à envoyer
}Message;//structure pour l'envoi de message

typedef struct Connexion{
  int idSocket;
  char username[25];
  int idFileSocket;
}Connexion;//strucutre des client idSocket, idFileSocket et username

typedef struct Fichier{
  Connexion client;
  FILE* file;//pointeur du fichier
  long int lenght;//taille du fichier
}Fichier;

typedef struct Salon {
  char nomSalon[25]; //nom du salon
  char descriptionSalon[150]; //description salon
  int personnesActuelles; //nombre de personnes actuellement dans le salon
  int* Socket_Client_Connecter;
  int personnesMax;//personnes max dans le salon
}Salon;//strucutre des Salons
//Socket_Client_Connecter=malloc(personnesMax*sizeof(int));

