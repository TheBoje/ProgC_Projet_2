/************************************************************************
 * Projet Numéro 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

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


void create_pipes_master(int *input, int *output); // Créer les pipes dans le master
void init_pipes_master(int input[], int output[]); // Initialise les pipes entre le master et le worker
void create_worker(int workerIn, int workerOut); // Clone le master et substitue le worker au master cloné
void close_pipes_master(int input[], int output[]); // Ferme les pipes entre le master et les workers

#endif
