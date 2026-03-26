# Explication des fonctions (`serveur.h` et `rover.h`)

Ce document explique le role de chaque fonction et comment les utiliser dans le projet.

## 1. Types utilises

- `Position` (dans `def.h`) : coordonnees `x` et `y`.
- `Rover` (dans `def.h`) :
  - `id` : identifiant du rover.
  - `socketConnexion` : etat/handle de connexion.
  - `position` : position actuelle.
  - `batterie` : niveau de batterie.
  - `statut` : texte d'etat.
  - `modeServeurActif` : indicateur de mode serveur/controle.

## 2. Fonctions de `serveur.h`

### 2.1 Fonctions declarees (a implementer plus tard)

Ces fonctions sont declarees, mais leur logique n'est pas encore ecrite dans les fichiers actuels :

- `demarrerServeur(void)` : demarrer le serveur Terre.
- `arreterServeur(void)` : arreter le serveur.
- `accepterConnexion(Rover *r)` : accepter la connexion d'un rover.
- `gererDeconnexion(Rover *r)` : gerer la deconnexion d'un rover.
- `mettreAJourPositionRover(Rover *r, Position p)` : mettre a jour la position d'un rover dans la carte.
- `enregistrerTresor(Position p)` : enregistrer un tresor detecte.
- `demanderLogs(Rover *r)` : demander les logs d'un rover.
- `archiverLogs(Rover *r, const char *logs)` : archiver les logs recus.
- `journaliserEvenement(const char *message)` : ecrire un evenement dans le journal.

Utilisation attendue :
- Tu les appelleras depuis la boucle serveur quand tu recevras des messages reseau.
- Exemple (plus tard, quand implementees) :

```c
accepterConnexion(&monRover);
journaliserEvenement("Rover connecte");
```

### 2.2 Fonctions deja implementees

#### `Position calculerNouvelleDestination(Rover *r)`

Ce que fait la fonction :
- Si `r == NULL`, elle retourne `{0, 0}`.
- Sinon, elle calcule une destination simple : `x + 1`, `y + 1`.

Pourquoi c'est utile :
- Fournit une destination de base sans dependre d'un etat serveur global.

Exemple :

```c
Position dest = calculerNouvelleDestination(&monRover);
```

#### `void envoyerNouvelleDestination(Rover *r, Position p)`

Ce que fait la fonction :
- Si `r == NULL`, ne fait rien.
- Sinon, elle affecte `p` a `r->position` et met `r->statut` a `"DESTINATION_RECUE"`.

Pourquoi c'est utile :
- Simule l'envoi/reception d'une destination au rover, en mettant son etat a jour localement.

Exemple :

```c
Position p = {10, 4};
envoyerNouvelleDestination(&monRover, p);
```

#### `int verifierBatterie(Rover *r)`

Ce que fait la fonction :
- Retourne `0` si `r == NULL`.
- Retourne `1` si `r->batterie > 20`, sinon `0`.

Pourquoi c'est utile :
- Permet de decider rapidement si le rover peut continuer sa mission.

Exemple :

```c
if (!verifierBatterie(&monRover)) {
    ordonnerRecharge(&monRover);
}
```

#### `void ordonnerRecharge(Rover *r)`

Ce que fait la fonction :
- Si `r == NULL`, ne fait rien.
- Active `modeServeurActif` et met le statut a `"RECHARGE"`.

Pourquoi c'est utile :
- Encapsule l'ordre de recharge en une seule fonction.

Exemple :

```c
ordonnerRecharge(&monRover);
```

## 3. Fonctions de `rover.h`

Dans l'etat actuel, `rover.h` contient des **declarations** uniquement (pas d'implementation dans ce fichier) :

- `FichierLog_ecrire`
- `Rover_seConnecter`, `Rover_seDeconnecter`
- `Rover_envoyerPosition`, `Rover_demanderNouvelleDestination`, `Rover_recevoirDestination`
- `Rover_seDeplacerVers`, `Rover_recharger`, `Rover_signalerTresor`
- `RoverAlpha_demarrerServeur`

Utilisation attendue :
- Ces fonctions seront appelees cote rover dans `rover.c` une fois implementees.
- Exemple cible :

```c
Rover_seConnecter(&monRover, IP_SERVEUR, PORT_TERRE);
Rover_envoyerPosition(&monRover);
Rover_demanderNouvelleDestination(&monRover);
```

## 4. Flux type d'utilisation

Un enchainement simple avec les fonctions deja disponibles :

```c
Rover monRover = {0};
monRover.position.x = 2;
monRover.position.y = 3;
monRover.batterie = 15;

if (!verifierBatterie(&monRover)) {
    ordonnerRecharge(&monRover);
} else {
    Position dest = calculerNouvelleDestination(&monRover);
    envoyerNouvelleDestination(&monRover, dest);
}
```

Ce flux est generique, sans creation d'objet serveur global, et reutilisable plus tard dans une vraie logique reseau.

## 5. Separation des responsabilites (mise a jour)

Pour eviter l'ambiguite :

- `modeServeurActif` : role du rover (ex: RoverAlpha passe en mode serveur).
- `etatRecharge` : etat metier de recharge (`0` = normal, `1` = recharge).

La fonction `ordonnerRecharge` met maintenant `etatRecharge = 1` et ne modifie plus `modeServeurActif`.
