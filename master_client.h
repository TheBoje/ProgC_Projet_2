/************************************************************************
 * Projet Num 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

#ifndef CLIENT_CRIBLE
#define CLIENT_CRIBLE

// On peut mettre ici des éléments propres au couple master/client :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (création tubes, écriture dans un tube,
//      manipulation de sémaphores, ...)

#define CONFIRMATION_STOP -2

// ordres possibles pour le master
#define ORDER_NONE 0
#define ORDER_STOP -1
#define ORDER_COMPUTE_PRIME 1
#define ORDER_HOW_MANY_PRIME 2
#define ORDER_HIGHEST_PRIME 3
#define ORDER_COMPUTE_PRIME_LOCAL 4 // ne concerne pas le master

#define FILE_KEY "master_client.h"
#define ID_CLIENTS 1
#define ID_MASTER_CLIENT 2
#define PIPE_MASTER_INPUT "pipe_master_input"
#define PIPE_MASTER_OUTPUT "pipe_master_output"
#define PIPE_CLIENT_INPUT "pipe_master_output"
#define PIPE_CLIENT_OUTPUT "pipe_master_input"
#define SIDE_MASTER 0
#define SIDE_CLIENT 1

// prendre mutex
void take_mutex(int sem_id);

// vendre mutex
void sell_mutex(int sem_id);

// ouvrir les tubes nommés
void open_pipe(int side, int res[]);

// fermer les tubes nommés
void close_pipe(int *fd);

// Destruction des tubes nommés
void destroy_pipe(char *pipe_name);

#endif
