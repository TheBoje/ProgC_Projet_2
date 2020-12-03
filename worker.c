#include "config.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

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
    bool hasNext;
} worker_data;

/************************************************************************
 * Fonctions secondaires
 ************************************************************************/
void init_worker_structure(worker_data *wd, int worker_prime_number, int unnamed_pipe_previous, int unnamed_pipe_master)
{
    wd->unnamed_pipe_previous = unnamed_pipe_previous;
    wd->unnamed_pipe_next = INIT_WORKER_NEXT_PIPE;
    wd->unnamed_pipe_master = unnamed_pipe_master;
    wd->worker_prime_number = worker_prime_number;
    wd->input_number = INIT_WORKER_VALUE;
    wd->hasNext = false;
}

void close_worker(worker_data *wd)
{
    printf("Worker [%d] closing\n", wd->worker_prime_number);
}

bool isPrime(worker_data *wd)
{
    return false; // TODO This
}

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

static void parseArgs(int argc, char *argv[], worker_data *wd)
{
    if (argc != 4)
        usage(argv[0], "Nombre d'arguments incorrect");

    init_worker_structure(wd, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
}

/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/

void loop(worker_data *wd)
{
    bool cont = true;

    while (cont)
    {
        printf("Worker [%d] en attente\n", wd->worker_prime_number);
        int read_number;
        int ret = read(wd->unnamed_pipe_previous, &read_number, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "worker - reading order failed\n");

        if (read_number == ORDRE_ARRET)
        {
            int toWrite = ORDRE_ARRET;
            ret = write(wd->unnamed_pipe_next, &toWrite, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "worker - failed writing stop order to next worker\n");

            close_worker(wd);
        }
        else
        {
            // TODO Ecrire au prochain pas au MASTER idiot
            // if (isPrime(wd))
            // {
            //     int toWrite = IS_PRIME;
            //     ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
            //     CHECK_RETURN(ret == RET_ERROR, "worker - failed writing result to master\n");
            // }
            // else if (!isPrime(wd))
            // {
            //     int toWrite = IS_NOT_PRIME;
            //     ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
            //     CHECK_RETURN(ret == RET_ERROR, "worker - failed writing result to master\n");
            // }
        }
    }
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

    worker_data wd;
    parseArgs(argc, argv, &wd);

    // Si on est créé c'est qu'on est un nombre premier
    // Envoyer au master un message positif pour dire
    // que le nombre testé est bien premier
    loop(&wd);

    // libérer les ressources : fermeture des files descriptors par exemple

    return EXIT_SUCCESS;
}
