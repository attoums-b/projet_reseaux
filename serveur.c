#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BACKLOG 10

#define OP_DEMANDER_ACTION 1
#define OP_OBSERVER 2

#define CMD_DESTINATION 1
#define CMD_RECHARGE 2
#define CMD_ETAT 3
#define CMD_ERREUR 99

typedef struct Position {
    int x;
    int y;
} Position;

typedef struct MessageReq {
    int op;
    int rover_id;
    int x;
    int y;
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

typedef struct MessageResp {
    int cmd;
    int x;
    int y;
    int batterie;
} MessageResp;

static ssize_t send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

static ssize_t recv_all(int fd, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n <= 0) return -1;
        recvd += (size_t)n;
    }
    return (ssize_t)recvd;
}

static void log_serveur(const char *msg) {
    FILE *f = fopen("journal_serveur.txt", "a");
    if (f == NULL) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

static void decode_req(const MessageReq *in, MessageReq *out) {
    out->op = ntohl(in->op);
    out->rover_id = ntohl(in->rover_id);
    out->x = ntohl(in->x);
    out->y = ntohl(in->y);
    out->batterie = ntohl(in->batterie);
}

static void encode_resp(const MessageResp *in, MessageResp *out) {
    out->cmd = htonl(in->cmd);
    out->x = htonl(in->x);
    out->y = htonl(in->y);
    out->batterie = htonl(in->batterie);
}

static int verifierBatterie(int batterie) {
    return batterie > 20;
}

static Position calculerNouvelleDestination(Position p) {
    Position d;
    d.x = p.x + 1;
    d.y = p.y + 1;
    return d;
}

static void traiter_client(int client_fd, const struct sockaddr_in *client_addr) {
    MessageReq req_net;
    char logbuf[256];

    snprintf(logbuf, sizeof(logbuf), "Connexion client depuis %s:%d",
             inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    puts(logbuf);
    log_serveur(logbuf);

    while (1) {
        MessageReq req;
        MessageResp resp;
        MessageResp resp_net;

        if (recv_all(client_fd, &req_net, sizeof(req_net)) < 0) {
            break;
        }
        decode_req(&req_net, &req);

        memset(&resp, 0, sizeof(resp));
        if (req.op == OP_DEMANDER_ACTION) {
            Position p;
            Position d;

            p.x = req.x;
            p.y = req.y;
            if (verifierBatterie(req.batterie)) {
                d = calculerNouvelleDestination(p);
                resp.cmd = CMD_DESTINATION;
                resp.x = d.x;
                resp.y = d.y;
                resp.batterie = req.batterie;

                snprintf(logbuf, sizeof(logbuf),
                         "Rover #%d -> DESTINATION (%d,%d)",
                         req.rover_id, d.x, d.y);
                log_serveur(logbuf);
            } else {
                resp.cmd = CMD_RECHARGE;
                resp.x = req.x;
                resp.y = req.y;
                resp.batterie = req.batterie;
                snprintf(logbuf, sizeof(logbuf),
                         "Rover #%d -> RECHARGE (batterie=%d)",
                         req.rover_id, req.batterie);
                log_serveur(logbuf);
            }
        } else if (req.op == OP_OBSERVER) {
            resp.cmd = CMD_ETAT;
            resp.x = req.x;
            resp.y = req.y;
            resp.batterie = req.batterie;
            snprintf(logbuf, sizeof(logbuf),
                     "Rover #%d observe etat pos=(%d,%d) bat=%d",
                     req.rover_id, req.x, req.y, req.batterie);
            log_serveur(logbuf);
        } else {
            resp.cmd = CMD_ERREUR;
        }

        encode_resp(&resp, &resp_net);
        if (send_all(client_fd, &resp_net, sizeof(resp_net)) < 0) {
            break;
        }
    }

    close(client_fd);
    log_serveur("Client deconnecte");
}

int main(int argc, char **argv) {
    int server_fd;
    struct sockaddr_in srv_addr;
    int port = DEFAULT_PORT;
    int opt = 1;

    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Port invalide.\n");
            return 1;
        }
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Serveur en ecoute sur le port %d\n", port);
    log_serveur("Serveur demarre");

    while (1) {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        traiter_client(client_fd, &client_addr);
    }

    close(server_fd);
    return 0;
}
