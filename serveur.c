
#ifndef SERVEUR_H
#define SERVEUR_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "def.h"
#include "serveur.h" 



void demarrerServeur(Serveur* srv, int port) {
    srv->port = port;
    srv->carte.nbTresors = 0;
    
    // Initialise tous les slots de rovers comme inactifs
    for(int i = 0; i < MAX_ROVERS; i++) {
        srv->carte.sessionsRovers[i].actif = 0;
    }

  
    srv->socketEcoute = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->socketEcoute == 0) {
        perror("Échec de la création du socket serveur");
        exit(EXIT_FAILURE);
    }

  
    int opt = 1;
    setsockopt(srv->socketEcoute, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in adresse;
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY; 
    adresse.sin_port = htons(srv->port);

    if (bind(srv->socketEcoute, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) {
        perror("Échec du bind");
        exit(EXIT_FAILURE);
    }

    // 5. Mise en écoute (Listen) - 3 connexions simultanées en attente max
    if (listen(srv->socketEcoute, 3) < 0) {
        perror("Échec du listen");
        exit(EXIT_FAILURE);
    }

    srv->enFonctionnement = 1;
    
    // Fait appel à la fonction d'Emmanuel pour journaliser
    char msg[100];
    sprintf(msg, "Serveur Terre Démarré et en écoute sur le port %d.", port);
    journaliserEvenement(msg); 
}

    srv->enFonctionnement = 0;
    
    // Fermeture du socket d'écoute principal
    close(srv->socketEcoute);
    
    // Fermeture de tous les sockets clients encore connectés
    for (int i = 0; i < MAX_ROVERS; i++) {
        if (srv->carte.sessionsRovers[i].actif) {
            close(srv->carte.sessionsRovers[i].idSocket);
            srv->carte.sessionsRovers[i].actif = 0;
        }
    }

    journaliserEvenement("Serveur Terre Arrêté.");



void accepterConnexion(Serveur* srv) {
    struct sockaddr_in adresseClient;
    socklen_t addrlen = sizeof(adresseClient);
    
    // Accepte la connexion (Création d'un nouveau socket dédié à ce rover)
    int new_socket = accept(srv->socketEcoute, (struct sockaddr*)&adresseClient, &addrlen);
    
    if (new_socket < 0) {
        perror("Erreur lors de l'acceptation de la connexion");
        return;
    }

    // Cherche un emplacement libre dans le tableau des rovers
    int placeTrouvee = 0;
    for (int i = 0; i < MAX_ROVERS; i++) {
        if (!srv->carte.sessionsRovers[i].actif) {
            // Assigne le socket au rover
            srv->carte.sessionsRovers[i].idSocket = new_socket;
            srv->carte.sessionsRovers[i].actif = 1;
            
            char msg[100];
            sprintf(msg, "Nouveau Rover accepté (Socket %d) depuis l'IP %s", 
                    new_socket, inet_ntoa(adresseClient.sin_addr));
            journaliserEvenement(msg);
            
            placeTrouvee = 1;
            break;
        }
    }

   
    if (!placeTrouvee) {
        journaliserEvenement("Connexion refusée : Nombre maximum de rovers atteint.");
        close(new_socket); // On rejette le rover
    }
}



void gererDeconnexion(Serveur* srv, int index) {
    int socketFerme = srv->carte.sessionsRovers[index].idSocket;
    
    // Ferme proprement la connexion réseau avec ce rover
    close(socketFerme);
    
    // Marque l'emplacement comme libre pour un futur rover
    srv->carte.sessionsRovers[index].actif = 0;
    
    char msg[100];
    sprintf(msg, "Rover (Socket %d) déconnecté proprement.", socketFerme);
    journaliserEvenement(msg);
} 
static inline Position calculerNouvelleDestination(Rover *r) {
    Position destination = {0, 0};

    if (r == NULL) {
        return destination;
    }

    destination.x = r->position.x + 1;
    destination.y = r->position.y + 1;
    return destination;
}

static inline void envoyerNouvelleDestination(Rover *r, Position p) {
    char message[64];
    int len;

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    /* L'ordre envoye est base sur la destination calculee serveur. */
    p = calculerNouvelleDestination(r);
    len = snprintf(message, sizeof(message), "Dirige aux coordonnées x:%d y:%d\n", p.x, p.y);
    if (len <= 0) {
        return;
    }

    send(r->socketConnexion, message, (size_t)len, 0);
}

static inline int verifierBatterie(Rover *r) {
    if (r == NULL) {
        return 0;
    }

    return r->batterie > 20;
}

static inline void ordonnerRecharge(Rover *r) {
    static const char message[] = "Recharge toi\n";

    if (r == NULL || r->socketConnexion < 0) {
        return;
    }

    send(r->socketConnexion, message, sizeof(message) - 1, 0);
}


#endif /* SERVEUR_H */