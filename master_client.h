#ifndef CLIENT_CRIBLE
#define CLIENT_CRIBLE

// On peut mettre ici des éléments propres au couple master/client :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (création tubes, écriture dans un tube,
//      manipulation de sémaphores, ...)

// ordres possibles pour le master
#define ORDER_NONE 0
#define ORDER_STOP -1
#define ORDER_COMPUTE_PRIME 1
#define ORDER_HOW_MANY_PRIME 2
#define ORDER_HIGHEST_PRIME 3
#define ORDER_COMPUTE_PRIME_LOCAL 4 // ne concerne pas le master

// bref n'hésitez à mettre nombre de fonctions avec des noms explicites
// pour masquer l'implémentation

#define FILE_KEY "master_client.h"
#define ID_CLIENTS 1
#define ID_MASTER_CLIENT 2
#define PIPE_MASTER_INPUT "pipe_master_input"
#define PIPE_MASTER_OUTPUT "pipe_master_output"
#define PIPE_CLIENT_INPUT "pipe_master_output"
#define PIPE_CLIENT_OUTPUT "pipe_master_input"

// SIGNATURES
// prendre mutex (dans master_client)
int take_mutex(int sem_id);

// vendre mutex (dans master_client)
int sell_mutex(int sem_id);

// ouvrir les tubes nommés (dans master_client)
int open_pipe(int *fd, int side);

// fermer les tubes nommés (dans master_client)
int close_pipe(int *fd);

// NON écriture sur le tube nommé (dans master_client)
// NON lecture sur le tube nommé (dans master_client)

// Initialisation sémaphores (dans master_client)
void init_sem(int sem_client_id, int sem_client_master_id);
// Initialisation des tubes nommés (dans master_client)
int init_pipes();
// Destruction des tubes nommés (dans master_client)
int destroy_pipes();

#endif
