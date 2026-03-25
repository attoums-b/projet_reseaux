/*Définir que les fonctions qu'utilise le rover pour éviter les dépendances circulaires*/

// methode de logs
#include "def.h"
void FichierLog_ecrire(Rover *r, const char *message);

// methode de rover a implementer
int Rover_seConnecter(Rover *r, const char *ip, int port);
void Rover_seDeconnecter(Rover *r);

void Rover_envoyerPosition(Rover *r);
void Rover_demanderNouvelleDestination(Rover *r);
void Rover_recevoirDestination(Rover *r, char *buffer);

void Rover_seDeplacerVers(Rover *r, Position p);
void Rover_recharger(Rover *r);
void Rover_signalerTresor(Rover *r);

// prototype de rover alpha
void RoverAlpha_demarrerServeur(Rover *r);