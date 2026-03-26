#ifndef SERVEUR_H
#define SERVEUR_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "def.h"

/* Fonctions serveur */
void demarrerServeur(void);
void arreterServeur(void);
void accepterConnexion(Rover *r);
void gererDeconnexion(Rover *r);
void mettreAJourPositionRover(Rover *r, Position p);
void enregistrerTresor(Position p);
void demanderLogs(Rover *r);
void archiverLogs(Rover *r, const char *logs);
void journaliserEvenement(const char *message);

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
