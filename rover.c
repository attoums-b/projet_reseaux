#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT_TERRE 8080
#define PORT_ALPHA 8081
#define IP_SERVEUR "127.0.0.1"

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

/* Non implementees pour le moment (conservees en declarations) */
void FichierLog_ecrire(Rover *r, const char *message);
void Rover_envoyerPosition(Rover *r);
void Rover_demanderNouvelleDestination(Rover *r);
void Rover_recevoirDestination(Rover *r, char *buffer);
void Rover_seDeplacerVers(Rover *r, Position p);
void Rover_signalerTresor(Rover *r);
void RoverAlpha_demarrerServeur(Rover *r);

/* Implementees */
int Rover_seConnecter(Rover *r, const char *ip, int port) {
    struct sockaddr_in serveurAddr;
    int sockfd;

    if (r == NULL || ip == NULL || port <= 0) {
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    memset(&serveurAddr, 0, sizeof(serveurAddr));
    serveurAddr.sin_family = AF_INET;
    serveurAddr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, ip, &serveurAddr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serveurAddr, sizeof(serveurAddr)) < 0) {
        close(sockfd);
        return -1;
    }

    r->socketConnexion = sockfd;
    r->modeServeurActif = 0;
    r->etatRecharge = 0;
    strncpy(r->statut, "CONNECTE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
    return 0;
}

void Rover_seDeconnecter(Rover *r) {
    if (r == NULL) {
        return;
    }

    if (r->socketConnexion >= 0) {
        close(r->socketConnexion);
        r->socketConnexion = -1;
    }

    strncpy(r->statut, "DECONNECTE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
}

void Rover_recharger(Rover *r) {
    if (r == NULL) {
        return;
    }

    r->batterie = 100;
    r->etatRecharge = 0;
    strncpy(r->statut, "CHARGE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
}
