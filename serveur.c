#include <stdio.h>
#include <sys/socket.h>

#define MAX_ROVERS 64
#define MAX_TRESORS 256
#define MAX_CHEMIN_LOG 256

typedef struct Position {
    int x;
    int y;
} Position;

typedef struct Rover {
    int id;
    int socketConnexion;
    Position position;
    int batterie;
    char statut[50];
    int modeServeurActif;
    int etatRecharge;
} Rover;

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

typedef struct IServeurTerre {
    int port;
    Rover *listeRovers[MAX_ROVERS];
    int nbRovers;
    CarteMars carte;
    FichierLog journal;
} IServeurTerre;

/* Non implementees pour le moment (conservees en declarations) */
void demarrerServeur(void);
void arreterServeur(void);
void accepterConnexion(Rover *r);
void gererDeconnexion(Rover *r);
/* Implementees */
Position calculerNouvelleDestination(Rover *r) {
    Position destination = {0, 0};

    if (r == NULL) {
        return destination;
    }

    destination.x = r->position.x + 1;
    destination.y = r->position.y + 1;
    return destination;
}

void envoyerNouvelleDestination(Rover *r, Position p) {
    char message[64];
    int len;

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    p = calculerNouvelleDestination(r);
    len = snprintf(message, sizeof(message), "DESTINATION %d %d\\n", p.x, p.y);
    if (len <= 0) {
        return;
    }

    send(r->socketConnexion, message, (size_t)len, 0);
}

int verifierBatterie(Rover *r) {
    if (r == NULL) {
        return 0;
    }

    return r->batterie > 20;
}

void ordonnerRecharge(Rover *r) {
    static const char message[] = "RECHARGE\\n";

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    send(r->socketConnexion, message, sizeof(message) - 1, 0);
}

/*Ma partie (emmanuel)*/

void mettreAJourPositionRover(Rover *r, Position p){
    if(r == NULL){
        return ; 
    }
    r->position = p;
    printf("Position du rover mise à jour: (%d, %d)\n", p.x , p.y);



}

void enregistrerTresor(Position p){
    static CarteMars carte; // simplification pédagogique

    if(carte.nbTresors >= MAX_TRESORS){
        printf("Max tresors atteint\n");
        return;
    }

    carte.tresors[carte.nbTresors] = p;
    carte.nbTresors++;

    printf("Tresor enregistre en (%d, %d)\n", p.x, p.y);
}

//fonctions permettant de demander les logs 
void demanderLogs(Rover *r){
    const char *message = "ENVOI_LOGS\n";

    if(r == NULL || r->socketConnexion < 0){
        return;
    }

    send(r->socketConnexion, message, strlen(message), 0);

    printf("Demande de logs envoyee au rover %d\n", r->id);
}


void archiverLogs(Rover *r, const char *logs){
    FILE *f = fopen("logs_serveur.txt", "a");

    if(f == NULL){
        printf("Erreur ouverture fichier logs\n");
        return;
    }

    fprintf(f, "Rover %d : %s\n", r->id, logs);
    fclose(f);

    printf("Logs archives pour rover %d\n", r->id);
}

void journaliserEvenement(const char *message){
    FILE *f = fopen("journal_serveur.txt", "a");

    if(f == NULL){
        return;
    }

    fprintf(f, "%s\n", message);
    fclose(f);

    printf("[LOG] %s\n", message);
}