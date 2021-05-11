#include "../Constante&Struct.h"
#include "fserveur.h"


int estConnecte(char* username){//renvoie 1 si l'username est déjà connecté sinon 0
  for (int i=0; i<nb_connexion; i++){
    if(strcmp(Liste_Connexion[i].username, username) == 0){
      return TRUE;
    }
  }
  return FALSE;
}
int estPresent(Salon nouveau){//renvoie 1 si le nom du salon est déja dans la liste sinon 0
  for (int i=0; i<nb_Salon; i++){
    if(strcmp(Liste_Salon[i].nomSalon, nouveau.nomSalon) == 0){
      return TRUE;
    }
  }
  return FALSE;

}

int resarchIndex(int* Tab_ID,int id,int taille){//renvoie l'index de l'id rentré dans un tableau sinon ERREUR
  int index=ERREUR;
  for(int i=0;i<taille;i++){
    if(Tab_ID[i]==id){                      
      index=i;
    }
  }
  return index;
}
void sendServerMsg(int client,char* msg){//envoie au client un message serveur rentrer
  Message msgServeur;
  strcpy(msgServeur.username,"Serveur");
  strcpy(msgServeur.msg,msg);
  send(client, &msgServeur, sizeof(Message),0);
}

void cSalon(int socketMsg,Salon salon){
  if ((nb_Salon < NB_SALON_MAX)) {
    
    if(!estPresent(salon)){
      salon.personnesActuelles=0;
      salon.Socket_Client_Connecter=calloc(salon.personnesMax,sizeof(int));
      Liste_Salon[nb_Salon]=salon;
      nb_Salon++;
      //TODO inserer fonction modification du fichier texte
    }else{
      sendServerMsg(socketMsg,"Nom du salon déjà pris");
    }
  } else {
    sendServerMsg(socketMsg,"Nombre maximal de salon atteint");
  }
}
void mSalon(int socketMsg,Salon old,Salon nouveau){
  if(estPresent(old)){
    for(int i=0;i<nb_Salon;i++){
      if (strcmp(Liste_Salon[i].nomSalon,old.nomSalon)==0){
        if(nouveau.personnesMax>Liste_Salon[i].personnesActuelles){
          nouveau.Socket_Client_Connecter=Liste_Salon[i].Socket_Client_Connecter;
          nouveau.personnesActuelles=Liste_Salon[i].personnesActuelles;
          Liste_Salon[i]=nouveau;
        }else{
          
          sendServerMsg(socketMsg,"ERREUR Limite de personne inférieur au personne actuellement connecté");
        }
        
      }
    }
  }
  else{
    sendServerMsg(socketMsg,"Salon existe pas");
  }
}
void dSalon(int socketMsg,Salon salon);
