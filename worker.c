#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "myassert.h"

#include "master_worker.h"

/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le worker
// a besoin : le nombre premier dont il a la charge, ...

// Liste :
// - Tube anonyme précédent -> self
// - Tube anonyme self -> suivant
// - Tube anonyme self -> master
// - Nombre cible (N)
// - Nombre premier dont il a la charge (P)
typedef struct worker_data
{
    int unnamed_pipe_previous;
    int unnamed_pipe_next;
    int unnamed_pipe_master;
    int worker_prime_number;
    int input_number;
} worker_data;

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void
usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <n> <fdIn> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <n> : nombre premier géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée pour tester un nombre\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie pour indiquer si un nombre est premier ou non\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static void parseArgs(int argc, char *argv[] /*, structure à remplir*/)
{
    if (argc != 4)
        usage(argv[0], "Nombre d'arguments incorrect");

    // remplir la structure
}
/************************************************************************
 * Fonctions secondaires
 ************************************************************************/

// check nombre premier

/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/

void loop(/* paramètres */)
{
    // boucle infinie :
    //    attendre l'arrivée d'un nombre à tester
    //      -> lecture sur le pipe (dans master_worker)
    //    si ordre d'arrêt
    //      -> lecture sur le pipe (dans master_worker)
    //       si il y a un worker suivant, transmettre l'ordre et attendre sa fin
    //       sortir de la boucle
    //    sinon c'est un nombre à tester, 4 possibilités :
    //           - le nombre est premier
    //              -> écriture sur le pipe (dans master_worker)
    //           - le nombre n'est pas premier
    //              -> écriture sur le pipe (dans master_worker)
    //           - s'il y a un worker suivant lui transmettre le nombre
    //              -> écriture sur le pipe worker worker (dans master_worker)
    //           - s'il n'y a pas de worker suivant, le créer
    //              -> fork et changer les données
}

/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char *argv[])
{
    parseArgs(argc, argv /*, structure à remplir*/);

    // Si on est créé c'est qu'on est un nombre premier
    // Envoyer au master un message positif pour dire
    // que le nombre testé est bien premier
    loop(/* paramètres */);

    // libérer les ressources : fermeture des files descriptors par exemple

    return EXIT_SUCCESS;
}
