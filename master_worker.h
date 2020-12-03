#ifndef MASTER_WORKER_H
#define MASTER_WORKER_H

// On peut mettre ici des éléments propres au couple master/worker :
//    - des constantes pour rendre plus lisible les comunications
//    - des fonctions communes (écriture dans un tube, ...)

// SIGNATURES
void create_pipes_master(int *input, int *output);
// Initialisation des tubes anonymes (dans master_worker)
void init_pipes_master();
// Création du premier worker (dans master_worker)
void create_worker();
// Ordre de fin du premier worker (dans master_worker)

#endif
