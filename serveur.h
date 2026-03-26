#ifndef SERVEUR_H
#define SERVEUR_H

#include <string.h>
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

/* Fonctions demandees: implementation generale */
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
    if (r == NULL) {
        return;
    }

    r->position = p;
    strncpy(r->statut, "DESTINATION_RECUE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
}

static inline int verifierBatterie(Rover *r) {
    if (r == NULL) {
        return 0;
    }

    return r->batterie > 20;
}

static inline void ordonnerRecharge(Rover *r) {
    if (r == 0) {
        return;
    }
    strncpy(r->statut, "RECHARGE", sizeof(r->statut) - 1);
    r->statut[sizeof(r->statut) - 1] = '\0';
}

#endif /* SERVEUR_H */
