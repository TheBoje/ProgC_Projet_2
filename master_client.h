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

// SIGNATURES
// prendre mutex (dans master_client)
// vendre mutex (dans master_client)
// ouvrir les tubes nommés (dans master_client)
// fermer les tubes nommés (dans master_client)
// écriture sur le tube nommé (dans master_client)
// lecture sur le tube nommé (dans master_client)
// Initialisation sémaphores (dans master_client)
// Initialisation des tubes nommés (dans master_client)
// Destruction des tubes nommés (dans master_client)

#endif
