#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080

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

typedef struct Rover {
    int id;
    int socketConnexion;
    Position position;
    int batterie;
    char statut[50];
} Rover;

typedef struct MessageReq {
    int op;
    int rover_id;
    int x;
    int y;
    int batterie;
} MessageReq;

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

static void encode_req(const MessageReq *in, MessageReq *out) {
    out->op = htonl(in->op);
    out->rover_id = htonl(in->rover_id);
    out->x = htonl(in->x);
    out->y = htonl(in->y);
    out->batterie = htonl(in->batterie);
}

static void decode_resp(const MessageResp *in, MessageResp *out) {
    out->cmd = ntohl(in->cmd);
    out->x = ntohl(in->x);
    out->y = ntohl(in->y);
    out->batterie = ntohl(in->batterie);
}

static void log_rover(const Rover *r, const char *msg) {
    FILE *f;
    char nom[64];
    if (r == NULL) return;
    snprintf(nom, sizeof(nom), "log_rover_%d.txt", r->id);
    f = fopen(nom, "a");
    if (f == NULL) return;
    fprintf(f, "%s\n", msg);
    fclose(f);
}

static int connecter_serveur(Rover *r, const char *ip, int port) {
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    r->socketConnexion = fd;
    strncpy(r->statut, "CONNECTE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
    return 0;
}

static int envoyer_requete(Rover *r, int op) {
    MessageReq req;
    MessageReq req_net;
    MessageResp resp_net;
    MessageResp resp;
    char logbuf[256];

    req.op = op;
    req.rover_id = r->id;
    req.x = r->position.x;
    req.y = r->position.y;
    req.batterie = r->batterie;

    encode_req(&req, &req_net);
    if (send_all(r->socketConnexion, &req_net, sizeof(req_net)) < 0) return -1;
    if (recv_all(r->socketConnexion, &resp_net, sizeof(resp_net)) < 0) return -1;

    decode_resp(&resp_net, &resp);

    if (resp.cmd == CMD_DESTINATION) {
        r->position.x = resp.x;
        r->position.y = resp.y;
        r->batterie = resp.batterie > 0 ? resp.batterie - 1 : 0;
        strncpy(r->statut, "ACTION_RECUE", sizeof(r->statut) - 1);
        r->statut[sizeof(r->statut) - 1] = '\0';
        snprintf(logbuf, sizeof(logbuf),
                 "Action recue: direction position (%d,%d)", resp.x, resp.y);
        log_rover(r, logbuf);
    } else if (resp.cmd == CMD_RECHARGE) {
        r->batterie = 100;
        strncpy(r->statut, "RECHARGE", sizeof(r->statut) - 1);
        r->statut[sizeof(r->statut) - 1] = '\0';
        log_rover(r, "Action recue: recharge");
    } else if (resp.cmd == CMD_ETAT) {
        snprintf(logbuf, sizeof(logbuf),
                 "Etat observe: pos=(%d,%d) bat=%d", resp.x, resp.y, resp.batterie);
        log_rover(r, logbuf);
    } else {
        log_rover(r, "Erreur serveur");
    }

    return 0;
}

int main(int argc, char **argv) {
    Rover r;
    const char *ip;
    int port = DEFAULT_PORT;
    int choix;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP_SERVEUR> <ID_ROVER> [PORT]\n", argv[0]);
        return 1;
    }
    ip = argv[1];
    r.id = atoi(argv[2]);
    if (argc >= 4) {
        port = atoi(argv[3]);
    }
    if (r.id <= 0) {
        fprintf(stderr, "ID rover invalide.\n");
        return 1;
    }

    memset(&r.position, 0, sizeof(r.position));
    r.batterie = 100;
    r.socketConnexion = -1;
    strncpy(r.statut, "INIT", sizeof(r.statut) - 1);
    r.statut[sizeof(r.statut) - 1] = '\0';

    if (connecter_serveur(&r, ip, port) != 0) {
        perror("connect");
        return 1;
    }

    printf("Rover #%d connecte a %s:%d\n", r.id, ip, port);
    while (1) {
        printf("\n--- MENU ROVER #%d ---\n", r.id);
        printf("1) Demander action au serveur\n");
        printf("2) Se deplacer manuellement\n");
        printf("3) Recharger manuellement (+%%)\n");
        printf("4) Observer (demande etat)\n");
        printf("5) Quitter\n");
        printf("Choix: ");

        if (scanf("%d", &choix) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            continue;
        }

        if (choix == 1) {
            if (envoyer_requete(&r, OP_DEMANDER_ACTION) != 0) {
                puts("Erreur communication serveur.");
                break;
            }
            printf("Nouvelle position: (%d,%d), batterie=%d\n", r.position.x, r.position.y, r.batterie);
        } else if (choix == 2) {
            int x;
            int y;
            printf("Nouvelle position x y: ");
            if (scanf("%d %d", &x, &y) == 2) {
                r.position.x = x;
                r.position.y = y;
                if (r.batterie > 0) r.batterie--;
                log_rover(&r, "Action executee: deplacement manuel");
            }
        } else if (choix == 3) {
            int p;
            printf("Pourcentage de recharge: ");
            if (scanf("%d", &p) == 1) {
                if (p < 0) p = 0;
                r.batterie += p;
                if (r.batterie > 100) r.batterie = 100;
                log_rover(&r, "Action executee: recharge manuelle");
            }
        } else if (choix == 4) {
            if (envoyer_requete(&r, OP_OBSERVER) != 0) {
                puts("Erreur communication serveur.");
                break;
            }
            printf("Etat rover: pos=(%d,%d) bat=%d statut=%s\n",
                   r.position.x, r.position.y, r.batterie, r.statut);
        } else if (choix == 5) {
            break;
        }
    }

    if (r.socketConnexion >= 0) close(r.socketConnexion);
    return 0;
}
