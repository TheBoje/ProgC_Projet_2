#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"
/************************************************************************
 * Idées
 ************************************************************************/
// TODO calcul du temps d'execution (time.deltaTime)

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin

// Liste
// - Mutex client section critique
// - Mutex client - master
// - Tube nommé lecture client
// - Tube nommé écriture client
// - Tube anonyme lecture worker
// - Tube anonyme écriture worker

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

/************************************************************************
 * Fonctions secondaires
 ************************************************************************/

// Envoie d'accusé de reception - ORDER_STOP
// Compute prime - ORDER_COMPUTE_PRIME (N)
// How many prime - ORDER_HOW_MANY_PRIME
// Return ORDER_HIGHEST_PRIME

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(/* paramètres */)
{
    // boucle infinie :
    // - ouverture des tubes (cf. rq client.c) (dans master_client)
    // - attente d'un ordre du client (via le tube nommé) (dans master_client)
    // - si ORDER_STOP
    //       . envoyer ordre de fin au premier worker et attendre sa fin (dans master_worker)
    //       . envoyer un accusé de réception au client
    // - si ORDER_COMPUTE_PRIME
    //       . récupérer le nombre N à tester provenant du client (dans master_client)
    //       . construire le pipeline jusqu'au nombre N-1 (si non encore fait) :
    //             il faut connaître le plus grand nombre (M) déjà enovoyé aux workers
    //             on leur envoie tous les nombres entre M+1 et N-1 (SQRT(N) ?)
    //             note : chaque envoie déclenche une réponse des workers
    //       . envoyer N dans le pipeline
    //       . récupérer la réponse
    //       . la transmettre au client
    // - si ORDER_HOW_MANY_PRIME
    //       . transmettre la réponse au client
    // - si ORDER_HIGHEST_PRIME
    //       . transmettre la réponse au client
    // - fermer les tubes nommés (dans master_client)
    // - attendre ordre du client avant de continuer (sémaphore : précédence
    //      -> lire pipe client (dans master_client)
    //          --> sémaphore
    // - revenir en début de boucle
    //
    // il est important d'ouvrir et fermer les tubes nommés à chaque itération
    // voyez-vous pourquoi ?
    // TODO Répondre : Sinon 2 clients peuvent écrire et lire en même tempss
}

/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char *argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    // - création des sémaphores
    //      -> init sémaphores
    // - création des tubes nommés
    //      -> init tubes nommés
    // - création du premier worker
    //      -> init tubes anonymes
    //      -> init premier worker

    // boucle infinie
    loop(/* paramètres */);

    // destruction des tubes nommés, des sémaphores, ...

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
