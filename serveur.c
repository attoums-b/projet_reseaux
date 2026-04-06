#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
void mettreAJourPositionRover(Rover *r, Position p);
void enregistrerTresor(Position p);
void demanderLogs(Rover *r);
void archiverLogs(Rover *r, const char *logs);
void journaliserEvenement(const char *message);


int serveurSocket;

void demarrerServeur(void){
    struct sockaddr_in serveurAddr;

    // 1. creation socket
    serveurSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serveurSocket < 0){
        perror("Erreur socket");
        return;
    }

    // 2. configuration adresse
    serveurAddr.sin_family = AF_INET;
    serveurAddr.sin_port = htons(8080); // port
    serveurAddr.sin_addr.s_addr = INADDR_ANY; // accepte tous

    // 3. bind
    if(bind(serveurSocket, (struct sockaddr*)&serveurAddr, sizeof(serveurAddr)) < 0){
        perror("Erreur bind");
        return;
    }

    // 4. listen
    if(listen(serveurSocket, 5) < 0){
        perror("Erreur listen");
        return;
    }

    printf("Serveur demarre sur le port 8080...\n");
}

void accepterConnexion(Rover *r){
    struct sockaddr_in clientAddr;
    socklen_t taille = sizeof(clientAddr);

    if(r == NULL){
        return;
    }

    // accept retourne un NOUVEAU socket pour communiquer
    r->socketConnexion = accept(serveurSocket, (struct sockaddr*)&clientAddr, &taille);

    if(r->socketConnexion < 0){
        perror("Erreur accept");
        return;
    }

    printf("Rover %d connecte !\n", r->id);
}

void gererDeconnexion(Rover *r){
    if(r == NULL){
        return;
    }

    if(r->socketConnexion >= 0){
        close(r->socketConnexion);
        r->socketConnexion = -1;
    }

    printf("Rover %d deconnecte\n", r->id);
}

void arreterServeur(void){
    if(serveurSocket >= 0){
        close(serveurSocket);
    }

    printf("Serveur arrete\n");
}

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