# Othello

This project consists of cerating an Othello game using the Socket API in C language.
The goal is to communicate between two players running locally on the same machine.


## Fonctionement / Prise de notes

Il y'a deux grandes étapes dans l'implémentation du jeu:
- La partie sysytème avec gestion des threads et des sockets
- La partie jeu avec la gestion des coups et des points

### La partie système :

On lance un programme en spécifiant son port d'écoute grâce à la ligne de code : ./othello <num_port_ecoute>
On lance deux programmes avec chacun un port d'écoute différent.

Chacun des programmes va s'initialiser en créant un thread avec une socket d'écoute sur son port d'écoute.

Lorsqu'on appuie sur le bouton "démarrer partie", le programme doit fermer son thread d'écoute avec sa socket d'écoute.
Il doit ensuite créer un nouveau thread avec une  nouvelle socket qui va se connecter sur l'autre programme (à l'aide de son port).
A partir de ce moment les deux programmes peuvent échanger des informations.
Il est donc utile de définir la facon dont les deux programmes vont s'échanger les deux informations (le protocole de comunication).
