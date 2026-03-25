/*Définir que les fonctions qu'utilise le serveur pour éviter les dépendances circulaires*/
#include "def.h"
void (*demarrerServeur)(void);
void (*arreterServeur)(void);
void (*accepterConnexion)(Rover *r);
void (*gererDeconnexion)(Rover *r);
Position (*calculerNouvelleDestination)(Rover *r);
void (*envoyerNouvelleDestination)(Rover *r, Position p);
int (*verifierBatterie)(Rover *r);
void (*ordonnerRecharge)(Rover *r);
void (*mettreAJourPositionRover)(Rover *r, Position p);
void (*enregistrerTresor)(Position p);
void (*demanderLogs)(Rover *r);
void (*archiverLogs)(Rover *r, const char *logs);
void (*journaliserEvenement)(const char *message);