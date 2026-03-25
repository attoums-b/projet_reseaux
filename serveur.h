#ifndef SERVEUR_H
#define SERVEUR_H

#define MAX_ROVERS 64
#define MAX_TRESORS 256
#define MAX_CHEMIN_LOG 256

/* La structure des positions*/
typedef struct Position {
    int x;
    int y;
} Position;

typedef struct Rover Rover;

/* la structure des fichiers logs*/
typedef struct FichierLog {
    char chemin[MAX_CHEMIN_LOG];
} FichierLog;

typedef struct EntreePositionRover {
    Rover *rover;
    Position position;
} EntreePositionRover;

typedef struct CarteMars {
    EntreePositionRover positionsRovers[MAX_ROVERS];
    int nbPositionsRovers;
    Position tresors[MAX_TRESORS];
    int nbTresors;
} CarteMars;

typedef struct ServeurTerre {
    int port;
    Rover *listeRovers[MAX_ROVERS];
    int nbRovers;
    CarteMars carte;
    FichierLog journal;
} ServeurTerre;

/*La structure de l'interface serveur */
typedef struct IServeurTerre {
    void (*demarrerServeur)(void);
    void (*arreterServeur)(void);
    void (*accepterConnexion)(Rover *r);
    void (*gererDeconnexion)(Rover *r);
    void (*recevoirPosition)(Rover *r, Position p);
    Position (*calculerNouvelleDestination)(Rover *r);
    void (*envoyerNouvelleDestination)(Rover *r, Position p);
    int (*verifierBatterie)(Rover *r);
    void (*ordonnerRecharge)(Rover *r);
    void (*mettreAJourPositionRover)(Rover *r, Position p);
    void (*enregistrerTresor)(Position p);
    void (*demanderLogs)(Rover *r);
    void (*archiverLogs)(Rover *r, const char *logs);
    void (*journaliserEvenement)(const char *message);
} IServeurTerre;

#endif /* SERVEUR_H */
