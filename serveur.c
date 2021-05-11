#include "Constante&Struct.h"
#include "fonction/fserveur.h"
int nb_connexion = 0;
int nb_Salon=0;

sem_t semaphore;
pthread_mutex_t the_mutex;

Connexion* Liste_Connexion;//Tableau des connexion

Salon* Liste_Salon;//Tableau des Salons


void* sendFile(void* file_v){
  Fichier *file = (Fichier*)file_v;
  char buffer[MEMOIRE_CACHE];
  int taillebuffer=MEMOIRE_CACHE;
  while(MEMOIRE_CACHE==taillebuffer){// Quand taillebuffer n'est plus égal à MEMOIRE_CACHE, on arrive a la fin du fichier
    taillebuffer=fread(buffer, 1, MEMOIRE_CACHE, (*file).file); // fread retourne la taille du fichier lu
    send((*file).client.idFileSocket, buffer, sizeof(char)*taillebuffer, 0);
    
  }
  fclose((*file).file);
  pthread_exit(NULL); 
}

void* recvFile(void* file_v){
  Fichier *file = (Fichier*)file_v;

  long int position = 0;
  char buffer[MEMOIRE_CACHE];
  int taillebuffer = MEMOIRE_CACHE;

  while((*file).lenght > position + MEMOIRE_CACHE){
    recv((*file).client.idFileSocket, buffer, sizeof(char)*MEMOIRE_CACHE, 0);
    
    position+=fwrite(buffer, 1, MEMOIRE_CACHE, (*file).file);
  }
  recv((*file).client.idFileSocket, buffer, sizeof(char)*MEMOIRE_CACHE, 0);
  fwrite(buffer, 1, (*file).lenght-position, (*file).file);
  printf("Fin\n");
  fclose((*file).file);
  pthread_exit(NULL);
}


void* Client(void* client_v){//fonction thread qui gère l'envoi des message
  char msg[CARACTERES_MAX];
  pthread_t fileTransfer;
  Fichier file;
  Connexion *clientpointeur = (Connexion*)client_v;
  Connexion client = (*clientpointeur);
  int Salon_Rejoint=ERREUR;
  
  
  printf("connecté ID :%d\n",client.idSocket);
  int reussi = 1;
  do{
    reussi = 1;
    recv(client.idSocket, msg, 25*sizeof(char), 0);//username du client qui vient de se connecter
    if(estConnecte(msg)){
      reussi = 0;
      printf("valeur de l'usernanme :%s erreur  \n",msg);
      send(client.idSocket, &reussi,sizeof(int), 0);//prévient client de l'erreur
    }
    

  }while(reussi == 0);//boucle tant qu'il n'a pas choisi un pseudo pas présent dans les personnes connectés
  
  
  reussi = 1;

  send(client.idSocket, &reussi,sizeof(int), 0);//prévient client de la réussite
  printf("Envoie de la réussite \n");
  printf("clien id %d",client.idSocket);
  for(int i=0;i<nb_connexion;i++){
    printf("%d\n",Liste_Connexion[i].idSocket);
    if(Liste_Connexion[i].idSocket==client.idSocket){
      pthread_mutex_lock(&the_mutex);
      printf("Attribution à la place : %d le pseudo %s\n",i,msg);
      strcpy(Liste_Connexion[i].username, msg);//attribution du pseudo a son idSocket
      pthread_mutex_unlock(&the_mutex);
      client.idFileSocket=Liste_Connexion[i].idFileSocket;
    }
  }
  char destinatire[25];
  Message text;
  strcpy(text.username,msg);
  
  int etat = TRUE;
  while(etat){

    recv(client.idSocket, msg, CARACTERES_MAX*sizeof(char), 0);//reception de la commande ou du message
    if(strcmp(msg, "/private") == 0)
    {
      recv(client.idSocket, msg, 25*sizeof(char), 0);
      printf("username :%s\n",msg);
      if(estConnecte(msg)){//vérification que le pseudo est Connecte
        strcpy(destinatire,msg);
        printf("destinataire trouvé\n");
        recv(client.idSocket, msg, CARACTERES_MAX*sizeof(char), 0);//reception du messsage
        printf("Message recu : %s\n",msg);      
        strcpy(text.msg,msg);
        printf("username de l'envoyeur : %s\n",text.username);
        printf("Message envoyé : %s\n",msg);
        for(int i=0;i<nb_connexion;i++){
          if(strcmp(Liste_Connexion[i].username,destinatire) == 0)
          {
            printf("destinatire: %s\n",destinatire );
            send(Liste_Connexion[i].idSocket, &text, sizeof(Message), 0);
          }
        }
        
      }else{
        recv(client.idSocket, msg, CARACTERES_MAX*sizeof(char), 0);
        sendServerMsg(client.idSocket,"Utilisateur non trouvé");
        
      }
    }


    else if(strcmp(msg, "/fin") == 0){
      printf("debut de boucle fermeture\n" );
      for(int i = 0;i<nb_connexion;i++){
        
        if(Liste_Connexion[i].idSocket==client.idSocket)
        {
          printf("%s fin de conexion\n", text.username );
          pthread_mutex_lock(&the_mutex);
          Liste_Connexion[i] = Liste_Connexion[nb_connexion-1];
          nb_connexion--;
          
          pthread_mutex_unlock(&the_mutex);
          sem_post(&semaphore);
          close(client.idSocket);
          etat = FALSE;
          
        }
      }

    }
    else if(strcmp(msg, "/fileSend") == 0){
      char chemin[CARACTERES_MAX];
      printf("Attente du chemin ...\n");
      recv(client.idSocket, chemin, CARACTERES_MAX*sizeof(char), 0);
      printf("%s\n",chemin);
      if(strcmp(chemin,"erreur") != 0){
        file.client=client;
        long int lenght;
        recv(client.idSocket, &lenght,sizeof(long int), 0);
        file.lenght = lenght;
        printf("%ld\n",lenght);
        file.file = fopen(chemin,"wb");
        pthread_create(&fileTransfer,0 , recvFile, (void*)&file);
        
      }
    }
    else if(strcmp(msg, "/fileRecv") == 0){
      char liste_fichiers[50][50];
      DIR* dp=opendir("fichiers");
      struct dirent* ep;
      int i=0;
      while (ep = readdir(dp) ) {
        if(strcmp(ep->d_name,".")!=0 && strcmp(ep->d_name,"..")!=0) {
            strcpy(liste_fichiers[i], ep->d_name);
            i++;
        }
      }
      send(client.idFileSocket, liste_fichiers, 50*50*sizeof(char), 0);
      char bufferChemin[20];
      recv(client.idFileSocket, bufferChemin, CARACTERES_MAX, 0);
      char chemin[60]="fichiers/";
      strcat(chemin, bufferChemin);
      chemin[strlen(chemin)-1]='\0';
      file.client=client;

      FILE* fp;
      fp=fopen(chemin ,"rb");
      long int lenght;
      if(fp==NULL){
        lenght=ERREUR;
        send(client.idFileSocket, &lenght, sizeof(long int), 0);
      }else{

        fseek(fp,0,SEEK_END); // Place le curseur a la fin du fichier
        lenght=ftell(fp);
        fseek(fp,0,SEEK_SET);
        printf("%ld\n",lenght);
        send(client.idFileSocket,&lenght,sizeof(long int),0);
        file.file=fp;
        pthread_create(&fileTransfer,0,sendFile,(void*)&file);
      }
    }
    else if(strcmp(msg, "/createSalon") == 0){
      if (strcmp(text.username, "admin")!=0){//TODO
        printf("Pas admin\n");
      }else {
        Salon nouveauSalon;
        recv(client.idSocket,&nouveauSalon,sizeof(Salon),0);
        cSalon(client.idSocket,nouveauSalon);
      }
    }
    else if(strcmp(msg, "/modifSalon") == 0){
       if (strcmp(text.username, "admin")!=0){
        printf("Pas admin\n");
      }else {
        char nomSalon[25];
        recv(client.idSocket,&nomSalon,25*sizeof(char),0);
        Salon ancienSalon;
        Salon nouveauSalon;
        strcpy(ancienSalon.nomSalon,nomSalon);
        recv(client.idSocket,&nouveauSalon,sizeof(Salon),0);
        mSalon(client.idSocket,ancienSalon,nouveauSalon);
      }
    }
    
    else if(strcmp(msg, "/deleteSalon") == 0){
      if (strcmp(text.username, "admin")!=0){
        printf("Pas admin\n");
      }else {
        Salon supprSalon;
        recv(client.idSocket,&supprSalon,sizeof(Salon),0);
        if (estPresent(supprSalon)) {
          for(int i=0;i<nb_Salon;i++){
            if (strcmp(Liste_Salon[i].nomSalon,supprSalon.nomSalon)==0){//TODO inserer fonction de modification fichier salon et mutex à faire
              Liste_Salon[i]=Liste_Salon[nb_Salon-1];
              nb_Salon--;
            }
          }
        } else {
          sendServerMsg(client.idSocket,"Le Salon n'existe pas");
          
        }
      }
    }

    else if(strcmp(msg, "/joinSalon") == 0){//commande pour rejoindre un salon
      char nom[25];
      send(client.idFileSocket,&nb_Salon,sizeof(int),0);
      printf("%d\n",nb_Salon);
      send(client.idFileSocket,Liste_Salon, nb_Salon*sizeof(Salon), 0);
      printf("%s  %s %d/%d\n",Liste_Salon[0].nomSalon,Liste_Salon[0].descriptionSalon,Liste_Salon[0].personnesActuelles, Liste_Salon[0].personnesMax);

      recv(client.idFileSocket,nom, 25*sizeof(char),0);
      int Reussi=ERREUR;
      for(int i=0;i<nb_Salon;i++){
        if(strcmp(nom,Liste_Salon[i].nomSalon)==0){
          if(Liste_Salon[i].personnesActuelles<Liste_Salon[i].personnesMax){
            if(Salon_Rejoint==ERREUR){
              pthread_mutex_lock(&the_mutex);
              Liste_Salon[i].Socket_Client_Connecter[Liste_Salon[i].personnesActuelles]=client.idSocket;
              Liste_Salon[i].personnesActuelles++;
              pthread_mutex_unlock(&the_mutex);
              Salon_Rejoint=i;
            }else{
              int index=resarchIndex(Liste_Salon[Salon_Rejoint].Socket_Client_Connecter,client.idSocket,Liste_Salon[Salon_Rejoint].personnesMax);//récupération de l'index du socket utilisateur dans le tableau de conexion du salon
              if(index==ERREUR){
                pthread_mutex_lock(&the_mutex);
                Liste_Salon[i].Socket_Client_Connecter[Liste_Salon[i].personnesActuelles]=client.idSocket;
                Liste_Salon[i].personnesActuelles++;
                pthread_mutex_unlock(&the_mutex);
                Salon_Rejoint=i;
              }else{
                pthread_mutex_lock(&the_mutex);
                Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[index]=Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[Liste_Salon[Salon_Rejoint].personnesActuelles-1];//écrasement de son id par le dernier conecté
                Liste_Salon[Salon_Rejoint].personnesActuelles--;//
                Salon_Rejoint=i;//attribution de l'index de son nouveau salon
                Liste_Salon[i].Socket_Client_Connecter[Liste_Salon[i].personnesActuelles]=client.idSocket;//rajout du socket utilisateur dans la liste des socket connecté
                Liste_Salon[i].personnesActuelles++;
                pthread_mutex_unlock(&the_mutex);
              }
            }
            Reussi=TRUE;
            sendServerMsg(client.idSocket,"Tu as rejoins le salon");
            

          }else{
            sendServerMsg(client.idSocket,"Plus de place dans le salon");
            Reussi=FALSE;
          }
        }
      }
      if(Reussi==ERREUR){
        sendServerMsg(client.idSocket,"Salon non trouvé");
        Reussi=FALSE;
      }
    }


    else if(strcmp(msg, "/quit") == 0){
      if(Salon_Rejoint==ERREUR){
        sendServerMsg(client.idSocket,"Aucun salon attribué");
              
      }else{
        int index=resarchIndex(Liste_Salon[Salon_Rejoint].Socket_Client_Connecter,client.idSocket,Liste_Salon[Salon_Rejoint].personnesMax);//récupération de l'index du socket utilisateur dans le tableau de conexion du salon
        if(index==ERREUR){
          Salon_Rejoint=ERREUR;
          sendServerMsg(client.idSocket,"Aucun salon attribué");
          
        }else{
          pthread_mutex_lock(&the_mutex);
          Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[index]=Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[Liste_Salon[Salon_Rejoint].personnesActuelles-1];//écrasement de son id par le dernier conecté
          Liste_Salon[Salon_Rejoint].personnesActuelles--;//
          pthread_mutex_unlock(&the_mutex);
          Salon_Rejoint=ERREUR;
          sendServerMsg(client.idSocket,"Salon quitté");
          
        }
      }
    }


    else if(strcmp(msg, "/all") == 0){//Si l'utilisateur souhaite envoyer un message à tous les clients
      recv(client.idSocket, msg, CARACTERES_MAX*sizeof(char), 0);//réception du messsage
      strcpy(text.msg,msg);
      for(int i = 0;i<nb_connexion;i++){
        if(Liste_Connexion[i].idSocket!=client.idSocket)
        {
          send(Liste_Connexion[i].idSocket, &text, sizeof(Message), 0);
        }
      }
    }

    
    else{
      if(Salon_Rejoint==ERREUR){
        sendServerMsg(client.idSocket,"Aucun salon rejoint tapez /joinSalon");
      }
      else{
        int index=resarchIndex(Liste_Salon[Salon_Rejoint].Socket_Client_Connecter, client.idSocket,Liste_Salon[Salon_Rejoint].personnesActuelles );
        if(index==ERREUR){
          Salon_Rejoint=ERREUR;
          sendServerMsg(client.idSocket,"Aucun salon rejoint tapez /joinSalon");
        }else{
          for(int i = 0;i<Liste_Salon[Salon_Rejoint].personnesActuelles;i++){
            if(Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[i]!=client.idSocket)
            {
              strcpy(text.msg,msg);
              send(Liste_Salon[Salon_Rejoint].Socket_Client_Connecter[i], &text, sizeof(Message), 0);
            }
          }
        }
      }
    }
  }
}



int main(int argc, char *argv[]) {
  pthread_mutex_init(&the_mutex,0);
  sem_init(&semaphore, 0, NB_CONNEXION_MAX);
  int dS = socket(PF_INET, SOCK_STREAM, 0); // socket creation
  
  // socket address configuration
  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(atoi(argv[1]));
  printf("socket adress configured\n");

  int dSFile = socket(PF_INET, SOCK_STREAM, 0); // socket creation
  
  // socket address configuration
  struct sockaddr_in adr;
  adr.sin_family = AF_INET;
  adr.sin_addr.s_addr = INADDR_ANY;
  adr.sin_port = htons(atoi(argv[1])+1);
  printf("socket adress File configured\n");


  bind(dS, (struct sockaddr*)&ad, sizeof(ad)); // makes the socket visible to everyone
  listen(dS, 7) ; // listens for incoming connections
  struct sockaddr_in aC; // client's address
  socklen_t lg = sizeof(struct sockaddr_in);
  printf("listening\n");

  bind(dSFile, (struct sockaddr*)&adr, sizeof(adr)); // makes the socket visible to everyone
  listen(dSFile, 5) ; // listens for incoming connections
  printf("listening file\n");

  pthread_t my_thread_Client[NB_CONNEXION_MAX];

  Liste_Connexion=malloc(NB_CONNEXION_MAX*sizeof(Connexion));
  int test = TRUE;
  Salon premier;
  premier.personnesActuelles=0;
  strcpy(premier.nomSalon,"Salon 1");
  strcpy(premier.descriptionSalon,"Ceci est un test");
  premier.personnesMax=PERSONNE_MAX;
    premier.Socket_Client_Connecter= calloc(PERSONNE_MAX,sizeof(int));
  Liste_Salon=malloc(5*sizeof(Salon));
  Liste_Salon[0]=premier;
  nb_Salon++;


  while(TRUE){
    sem_wait(&semaphore);
    int dSC = accept(dS, (struct sockaddr*) &aC,&lg); // accepts the connection
    send(dSC,&test,sizeof(int),0);
    Connexion client;
    client.idSocket=dSC;
  
    int dSCfile = accept(dSFile, (struct sockaddr*) &aC,&lg);
    client.idFileSocket=dSCfile;

    
    pthread_mutex_lock(&the_mutex);
    Liste_Connexion[nb_connexion] = client;
    nb_connexion++;
    pthread_mutex_unlock(&the_mutex);  
    printf("%d connection accepted\n",nb_connexion);
    
    pthread_create(&my_thread_Client[nb_connexion],0, Client, (void*)&client);
  
  }
  shutdown(dS, 2);
  printf("shutdown\n");
}

