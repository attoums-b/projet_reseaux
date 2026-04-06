#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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


void FichierLog_ecrire(Rover *r, const char *message) {
    FILE *f;
    char nomFichier[64];
    time_t t;
    struct tm *tm_info;

    if (r == NULL || message == NULL) {
        return;
    }

    /* Creation du nom de fichier propre a ce rover */
    snprintf(nomFichier, sizeof(nomFichier), "log_rover_%d.txt", r->id);
    
    f = fopen(nomFichier, "a");
    if (f == NULL) {
        return;
    }

    /* Recuperation de l'heure actuelle */
    time(&t);
    tm_info = localtime(&t);

    /* Formatage et ecriture du log */
    fprintf(f, "[%02d:%02d:%02d] Bat:%d%% Pos:(%d,%d) | %s\n",
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            r->batterie, r->position.x, r->position.y, message);
            
    fclose(f);
    
    /* Affichage optionnel dans la console pour le debug */
    printf("[ROVER %d] %s\n", r->id, message);
}

void Rover_envoyerPosition(Rover *r) {
    char requete[128];
    int len;

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    /* Formatage de la demande  */
    len = snprintf(requete, sizeof(requete), "REQ_POS:%d:%d:%d", 
                   r->position.x, r->position.y, r->batterie);
                   
    if (len > 0) {
        send(r->socketConnexion, requete, (size_t)len, 0);
    }
}

void Rover_demanderNouvelleDestination(Rover *r) {
    if (r == NULL) {
        return;
    }

    FichierLog_ecrire(r, "Demande de nouvelle destination en cours...");
    Rover_envoyerPosition(r);
}

void Rover_recevoirDestination(Rover *r, char *buffer) {
    Position dest;

    if (r == NULL || buffer == NULL) {
        return;
    }

    /* Analyse de la reponse envoyee par le serveur */
    if (strncmp(buffer, "CMD_RECHARGE", 12) == 0) {
        FichierLog_ecrire(r, "Ordre de rechargement recu depuis la Terre.");
        Rover_recharger(r);
    } 
    else if (strncmp(buffer, "CMD_GO", 6) == 0) {
        /* Extraction des coordonnees de destination */
        if (sscanf(buffer, "CMD_GO:%d:%d", &dest.x, &dest.y) == 2) {
            Rover_seDeplacerVers(r, dest);
        }
    }
}

void Rover_signalerTresor(Rover *r) {
    char message[128];
    int len;

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    len = snprintf(message, sizeof(message), "NOTIF_TRESOR:%d:%d", 
                   r->position.x, r->position.y);
                   
    if (len > 0) {
        send(r->socketConnexion, message, (size_t)len, 0);
        FichierLog_ecrire(r, "Tresor detecte et signale a la Terre !");
    }
}

void Rover_seDeplacerVers(Rover *r, Position p) {
    if (r == NULL) {
        return;
    }

    /* Mise a jour du statut securisee */
    strncpy(r->statut, "EN_DEPLACEMENT", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
    FichierLog_ecrire(r, "Debut du deplacement...");

    while (r->position.x != p.x || r->position.y != p.y) {
        if (r->batterie <= 0) {
            FichierLog_ecrire(r, "Deplacement stoppe : Batterie vide !");
            break;
        }

        /* Logique de deplacement  */
        if (r->position.x < p.x) {
            r->position.x++;
        } else if (r->position.x > p.x) {
            r->position.x--;
        } else if (r->position.y < p.y) {
            r->position.y++;
        } else if (r->position.y > p.y) {
            r->position.y--;
        }

        r->batterie--;

        /*trouver un tresor  */
        if ((rand() % 5) == 0) {
            Rover_signalerTresor(r);
        }

        /* Simule le temps de deplacement 3 srec */
        usleep(300000); 
    }

    /* Retour a l'etat initial */
    strncpy(r->statut, "CONNECTE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
    FichierLog_ecrire(r, "Arrive a destination.");
}

void RoverAlpha_demarrerServeur(Rover *r) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[256];

    if (r == NULL) {
        return;
    }

    FichierLog_ecrire(r, ">>> BASCULE EN MODE SERVEUR ALPHA (Secours) <<<");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return;
    }

    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_ALPHA);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_fd);
        FichierLog_ecrire(r, "Echec du Bind Alpha.");
        return;
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return;
    }

    /* Met a jour le statut du Rover qui devient hebergeur */
    r->modeServeurActif = 1;
    strncpy(r->statut, "SERVEUR_ALPHA", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';

    /* Boucle infinie d'ecoute d'urgence */
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_socket >= 0) {
            
            /* Lit la requete pour vider le buffer du socket */
            read(client_socket, buffer, sizeof(buffer) - 1);
            
            /* Le cahier des charges exige une reponse systematique : Recharge et attends */
            send(client_socket, "CMD_RECHARGE", 12, 0);
            
            /* Coupe la connexion apres l'envoi de l'ordre */
            close(client_socket);
        }
    }
}