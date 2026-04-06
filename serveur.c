#include <stdio.h>
#include <stdlib.h>
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

/* AJOUT: etat global de la simulation locale */
static Rover gRovers[MAX_ROVERS];
static int gNbRovers = 0;
static int gIndexAlpha = 0;
static int gTerreKO = 0;

/* AJOUT: ecrit une ligne dans le log du rover Alpha */
static void logAlpha(const char *message) {
    FILE *f = fopen("log_rover_alpha.txt", "a");
    if (f == NULL) {
        return;
    }
    fprintf(f, "%s\n", message);
    fclose(f);
}

/* AJOUT: ecrit une ligne dans le log du rover cible (log_rover_<id>.txt) */
static void logRover(int roverId, const char *message) {
    FILE *f;
    char nom[64];

    snprintf(nom, sizeof(nom), "log_rover_%d.txt", roverId);
    f = fopen(nom, "a");
    if (f == NULL) {
        return;
    }
    fprintf(f, "%s\n", message);
    fclose(f);
}

/* AJOUT: trace explicitement l'ordre envoye par Alpha quand Terre est KO */
static void logOrdreAlpha(int roverId, const char *ordre, Position p) {
    char msg[256];
    if (strcmp(ordre, "DESTINATION") == 0) {
        snprintf(msg, sizeof(msg),
                 "Alpha -> rover #%d : DESTINATION (%d,%d)",
                 roverId, p.x, p.y);
    } else {
        snprintf(msg, sizeof(msg),
                 "Alpha -> rover #%d : %s",
                 roverId, ordre);
    }
    logAlpha(msg);
}

/* AJOUT: initialise tous les rovers avec position (0,0), batterie 100, statut INIT */
static void initialiserRovers(int nb) {
    int i;
    if (nb < 1) nb = 1;
    if (nb > MAX_ROVERS) nb = MAX_ROVERS;

    gNbRovers = nb;
    for (i = 0; i < gNbRovers; ++i) {
        memset(&gRovers[i], 0, sizeof(Rover));
        gRovers[i].id = i + 1;
        gRovers[i].socketConnexion = -1;
        gRovers[i].position.x = 0;
        gRovers[i].position.y = 0;
        gRovers[i].batterie = 100;
        strncpy(gRovers[i].statut, "INIT", sizeof(gRovers[i].statut) - 1);
        gRovers[i].statut[sizeof(gRovers[i].statut) - 1] = '\0';
    }
}

/* AJOUT: choix automatique du rover Alpha (le rover #1) + message de confirmation */
static void choisirAlphaAutomatique(void) {
    char msg[128];
    gIndexAlpha = 0; /* plus simple: le rover 1 devient alpha */
    snprintf(msg, sizeof(msg), "Rover Alpha choisi automatiquement: #%d", gRovers[gIndexAlpha].id);
    printf("%s\n", msg);
    journaliserEvenement(msg);
    logAlpha(msg);
}

/* AJOUT: traitement simple d'une demande d'action d'un rover */
static void actionDemanderAuServeur(Rover *r) {
    Position p;
    char logMsg[256];

    if (r == NULL) {
        return;
    }

    if (verifierBatterie(r)) {
        p = calculerNouvelleDestination(r);
        mettreAJourPositionRover(r, p);
        strncpy(r->statut, "ACTION_RECUE", sizeof(r->statut) - 1);
        r->statut[sizeof(r->statut) - 1] = '\0';
        printf("Rover #%d: nouvelle destination (%d,%d)\n", r->id, p.x, p.y);

        snprintf(logMsg, sizeof(logMsg),
                 "Demande au rover #%d de se rendre a la position (%d,%d)",
                 r->id, p.x, p.y);
        if (gTerreKO) {
            /* AJOUT: en mode KO, Alpha journalise ses propres ordres */
            logOrdreAlpha(r->id, "DESTINATION", p);
            journaliserEvenement("Terre KO: Alpha a traite une demande rover");
        } else {
            /* AJOUT: en mode normal, le serveur Terre journalise l'ordre */
            journaliserEvenement(logMsg);
        }

        /* AJOUT: log du rover sur l'action recue puis executee */
        snprintf(logMsg, sizeof(logMsg),
                 "Action recue: direction position (%d,%d)", p.x, p.y);
        logRover(r->id, logMsg);
        snprintf(logMsg, sizeof(logMsg),
                 "Action executee: position atteinte (%d,%d)", r->position.x, r->position.y);
        logRover(r->id, logMsg);
    } else {
        ordonnerRecharge(r);
        strncpy(r->statut, "RECHARGE", sizeof(r->statut) - 1);
        r->statut[sizeof(r->statut) - 1] = '\0';
        printf("Rover #%d: ordre de recharge.\n", r->id);

        if (gTerreKO) {
            Position pVide = {0, 0};
            /* AJOUT: ordre de recharge emis par Alpha en mode KO */
            logOrdreAlpha(r->id, "RECHARGE", pVide);
            journaliserEvenement("Terre KO: Alpha a ordonne une recharge");
        } else {
            snprintf(logMsg, sizeof(logMsg),
                     "Demande au rover #%d: RECHARGE", r->id);
            journaliserEvenement(logMsg);
        }
        /* AJOUT: log du rover sur la recharge recue/executee */
        logRover(r->id, "Action recue: recharge");
        logRover(r->id, "Action executee: batterie en recharge");
    }
}

/* AJOUT: affiche la "fiche" minimaliste d'un rover (position + statut) */
static void afficherObservationRover(const Rover *r) {
    if (r == NULL) {
        return;
    }
    printf("Rover #%d | Position=(%d,%d) | Statut=%s\n",
           r->id, r->position.x, r->position.y, r->statut);
}

/* AJOUT: sous-menu utilisateur pour choisir un rover et lancer ses actions */
static void menuRoverUtilisateur(void) {
    int index;
    while (1) {
        int choix;
        Rover *r;

        printf("Quel est le numero du rover (1..%d) ? (0 = retour menu accueil) ", gNbRovers);
        if (scanf("%d", &index) != 1) {
            printf("Numero invalide.\n");
            return;
        }
        if (index == 0) {
            return;
        }
        if (index < 1 || index > gNbRovers) {
            printf("Numero invalide.\n");
            continue;
        }

        r = &gRovers[index - 1];
        while (1) {
            printf("\n--- MENU ROVER #%d ---\n", r->id);
            printf("1) Demander action\n");
            printf("2) Observer\n");
            printf("3) Changer de rover\n");
            printf("4) Retour menu accueil\n");
            printf("Choix: ");

            if (scanf("%d", &choix) != 1) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF) {
                }
                continue;
            }

            if (choix == 1) {
                actionDemanderAuServeur(r);
            } else if (choix == 2) {
                afficherObservationRover(r);
            } else if (choix == 3) {
                break;
            } else if (choix == 4) {
                return;
            }
        }
    }
}

/* AJOUT: active/desactive la panne Terre et journalise la prise de relais Alpha */
static void simulerServeurTerreKO(void) {
    gTerreKO = !gTerreKO;
    if (gTerreKO) {
        printf("Simulation Serveur Terre KO: ACTIVE\n");
        logAlpha("rover terre chaos, je prends le relais");
    } else {
        printf("Simulation Serveur Terre KO: DESACTIVEE\n");
    }
}

/* AJOUT: menu principal Alpha */
int main(void) {
    int nb;

    printf("Nombre de rovers: ");
    if (scanf("%d", &nb) != 1) {
        printf("Entree invalide.\n");
        return 1;
    }

    initialiserRovers(nb);
    choisirAlphaAutomatique();

    while (1) {
        int choix;

        printf("\n--- MENU ROVER ALPHA ---\n");
        printf("1) Utilisateur rover\n");
        printf("2) Simuler serveur Terre KO\n");
        printf("3) Quitter\n");
        printf("Choix: ");

        if (scanf("%d", &choix) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            continue;
        }

        if (choix == 1) {
            menuRoverUtilisateur();
        } else if (choix == 2) {
            simulerServeurTerreKO();
        } else if (choix == 3) {
            break;
        }
    }

    return 0;
}
