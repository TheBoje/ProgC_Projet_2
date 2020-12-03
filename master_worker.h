#ifndef MASTER_WORKER_H
#define MASTER_WORKER_H

#define FIRST_PRIME_NUMBER 2
// On peut mettre ici des éléments propres au couple master/worker :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (écriture dans un tube, ...)

// SIGNATURES
void create_pipes_master(int *input, int *output);
// Initialisation des tubes anonymes (dans master_worker)
void init_pipes_master(int input[], int output[]);
// Création du premier worker (dans master_worker)
void create_worker(int workerIn, int workerOut);
// Ordre de fin du premier worker (dans master_worker)

#endif
