#ifndef ROVER_H
#define ROVER_H


// constante
#define PORT_TERRE 8080
#define PORT_ALPHA 8081
#define IP_SERVEUR "127.0.0.1"

// structuer
typedef struct {
    int id;
    int socketConnexion;
    Position position;
    int batterie;
    char statut[50];
    int modeServeurActif; 
} Rover;


// methode de logs 
void FichierLog_ecrire(Rover* r, const char* message);


// methode de rover a implementer 

int Rover_seConnecter(Rover* r, const char* ip, int port);
void Rover_seDeconnecter(Rover* r);

void Rover_envoyerPosition(Rover* r);
void Rover_demanderNouvelleDestination(Rover* r);
void Rover_recevoirDestination(Rover* r, char* buffer);

void Rover_seDeplacerVers(Rover* r, Position p);
void Rover_recharger(Rover* r);
void Rover_signalerTresor(Rover* r);


// prototype de rover alpha
void RoverAlpha_demarrerServeur(Rover* r);

#endif // ROVER_H