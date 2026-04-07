#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BACKLOG 10

#define OP_DEMANDER_ACTION 1
#define OP_OBSERVER 2
#define OP_SIMULER_KO 9

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
} MessageReq;

typedef struct MessageResp {
    int cmd;
    int x;
    int y;
    int batterie;
} MessageResp;

static int gNbRoversMax = 1;
static int gRoverAlphaId = 1;
static int gTerreKO = 0;

static ssize_t send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n <= 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

static ssize_t recv_all(int fd, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n <= 0) {
            return -1;
        }
        recvd += (size_t)n;
    }
    return (ssize_t)recvd;
}

static void log_serveur(const char *msg) {
    FILE *f = fopen("journal_serveur.txt", "a");
    if (f == NULL) {
        return;
    }
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
        if (req.rover_id < 1 || req.rover_id > gNbRoversMax) {
            resp.cmd = CMD_ERREUR;
            encode_resp(&resp, &resp_net);
            if (send_all(client_fd, &resp_net, sizeof(resp_net)) < 0) {
                break;
            }
            continue;
        }

        if (req.op == OP_DEMANDER_ACTION) {
            Position p;
            Position d;

            p.x = req.x;
            p.y = req.y;
            if (gTerreKO) {
                resp.cmd = CMD_RECHARGE;
                resp.x = req.x;
                resp.y = req.y;
                resp.batterie = req.batterie;
                snprintf(logbuf, sizeof(logbuf),
                         "Terre KO: Alpha #%d ordonne RECHARGE au rover #%d",
                         gRoverAlphaId, req.rover_id);
                log_serveur(logbuf);
                {
                    FILE *fa = fopen("log_rover_alpha.txt", "a");
                    if (fa != NULL) {
                        fprintf(fa, "Alpha -> rover #%d : RECHARGE\n", req.rover_id);
                        fclose(fa);
                    }
                }
            } else if (verifierBatterie(req.batterie)) {
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
        } else if (req.op == OP_SIMULER_KO) {
            resp.cmd = CMD_ETAT;
            gTerreKO = !gTerreKO;
            resp.x = 0;
            resp.y = 0;
            resp.batterie = gTerreKO;
            if (gTerreKO) {
                log_serveur("Simulation Terre KO active");
                {
                    FILE *fa = fopen("log_rover_alpha.txt", "a");
                    if (fa != NULL) {
                        fprintf(fa, "rover terre chaos, je prends le relais\n");
                        fclose(fa);
                    }
                }
            } else {
                log_serveur("Simulation Terre KO desactivee");
            }
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
    int nb;

    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Port invalide.\n");
            return 1;
        }
    }

    printf("Nombre de rovers a gerer: ");
    if (scanf("%d", &nb) != 1 || nb < 1) {
        fprintf(stderr, "Nombre invalide.\n");
        return 1;
    }
    gNbRoversMax = nb;
    gRoverAlphaId = 1;
    printf("Rover Alpha choisi automatiquement: #%d\n", gRoverAlphaId);

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
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        traiter_client(client_fd, &client_addr);
    }

    close(server_fd);
    return 0;
}
