#ifndef MASTER_WORKER_H
#define MASTER_WORKER_H

#define FIRST_PRIME_NUMBER 2
// On peut mettre ici des éléments propres au couple master/worker :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (écriture dans un tube, ...)
#define INIT_WORKER_VALUE -1
#define INIT_WORKER_NEXT_PIPE -1
#define ORDRE_ARRET -2
#define STOP_SUCCESS -3
#define HOWMANY -4
#define HIGHEST -5
#define IS_PRIME -6
#define IS_NOT_PRIME -7

// Macro permettant de tester le retour de fonctions
#define CHECK_RETURN(c, m)  \
    if (c)                  \
    {                       \
        TRACE(m);           \
        exit(EXIT_FAILURE); \
    }
// SIGNATURES
void create_pipes_master(int *input, int *output);
// Initialisation des tubes anonymes (dans master_worker)
void init_pipes_master(int input[], int output[]);
// Création du premier worker (dans master_worker)
void create_worker(int workerIn, int workerOut);
// Ordre de fin du premier worker (dans master_worker)

#endif
