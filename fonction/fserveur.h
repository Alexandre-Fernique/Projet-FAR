


//VARIABLE GLOBAL

extern int nb_connexion;
extern int nb_Salon;

extern sem_t semaphore;
extern pthread_mutex_t the_mutex;

extern Connexion* Liste_Connexion;//Tableau des connexion

extern Salon* Liste_Salon;//Tableau des Salons

//FONCTION


int estConnecte(char* username);
int estPresent(Salon nouveau);
int resarchIndex(int* Tab_ID,int id,int taille);
void sendServerMsg(int client,char* msg);

void cSalon(int socketMsg,Salon salon);
void mSalon(int socketMsg,Salon old,Salon nouveau);
void dSalon(int socketMsg,Salon salon);
