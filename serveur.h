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


Position calculerNouvelleDestination(Rover *r);

void envoyerNouvelleDestination(Rover *r, Position p);

int verifierBatterie(Rover *r);

void ordonnerRecharge(Rover *r);