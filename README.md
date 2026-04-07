# Projet Réseaux - Rover Mars (C)

Simulation client/serveur en C pour piloter des rovers à distance.

- `serveur.c` : serveur Terre (TCP)
- `rover.c` : client rover (TCP)

Le projet applique les notions vues en cours :
- sockets TCP (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`),
- représentation externe des données avec `htonl` / `ntohl` (principe XDR/RPC simplifié).

## Fonctionnalités

- Connexion d'un rover distant au serveur
- Demande d'action (`DESTINATION` ou `RECHARGE` selon batterie)
- Observation de l'état rover
- Simulation `Serveur Terre KO` (Alpha prend le relais logique)
- Logs:
  - serveur: `journal_serveur.txt`
  - rover alpha: `log_rover_alpha.txt`
  - rover i: `log_rover_<id>.txt`

## Structure des messages

### Requête client -> serveur (`MessageReq`)
- `op` : opération (`OP_DEMANDER_ACTION`, `OP_OBSERVER`, `OP_SIMULER_KO`)
- `rover_id`
- `x`, `y`
- `batterie`

### Réponse serveur -> client (`MessageResp`)
- `cmd` : commande (`CMD_DESTINATION`, `CMD_RECHARGE`, `CMD_ETAT`, `CMD_ERREUR`)
- `x`, `y`
- `batterie`

## Compilation

```bash
gcc -Wall -Wextra -Werror serveur.c -o serveur_main
gcc -Wall -Wextra -Werror rover.c -o rover_main
```

## Exécution

### Test local (même machine)
Terminal 1 (serveur):
```bash
./serveur_main 8080
```

Terminal 2 (client):
```bash
./rover_main 127.0.0.1 1 8080
```

### Test sur 2 machines

#### Machine serveur
```bash
./serveur_main 8080
hostname -I
```
Récupère l'IP LAN (ex: `172.20.10.14`).

#### Machine client
```bash
./rover_main <IP_SERVEUR> 1 8080
```
Exemple:
```bash
./rover_main 172.20.10.14 1 8080
```

## Vérifications réseau utiles

```bash
ping -c 3 <IP_SERVEUR>
telnet <IP_SERVEUR> 8080
```

Si la connexion échoue:
- vérifier que les 2 machines sont sur le même réseau,
- vérifier que le port `8080` est autorisé par le pare-feu,
- vérifier que le serveur est bien lancé.

## Notes

- Le serveur demande le nombre de rovers au démarrage.
- Le rover Alpha est attribué automatiquement.
- Le menu client permet aussi le switch d'ID rover et la simulation KO.

## Auteur
Cyriaque Koffi
Justin AWAD
Emmanuel Blond

Projet pédagogique - L3 Réseaux.
