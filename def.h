#ifndef DEF_H
#define DEF_H

#define PORT_TERRE 8080
#define PORT_ALPHA 8081
#define IP_SERVEUR "127.0.0.1"
#define MAX_ROVERS 64
#define MAX_TRESORS 256
#define MAX_CHEMIN_LOG 256

/* La structure des positions*/
typedef struct Position {
    int x;
    int y;
} Position;


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


/*La structure de l'interface serveur */
typedef struct IServeurTerre {
    int port;
    Rover *listeRovers[MAX_ROVERS];
    int nbRovers;
    CarteMars carte;
    FichierLog journal;
} IServeurTerre;

/* typedef struct Rover Rover */
typedef struct Rover {
    int id;
    int socketConnexion;
    Position position;
    int batterie;
    char statut[50];
    int modeServeurActif;
};

#endif /* DEF_H */
