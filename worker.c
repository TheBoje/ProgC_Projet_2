/************************************************************************
 * Projet Numéro 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

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
// Structure de données de chaque worker.
// worker_prime_number représente l'entier dont le worker à la charge. Cet entier est compris entre 2 et input_number
// input_number représente le numéro dont il faut tester la primalité
// hasNext décrit si le worker possède un worker suivant ou non
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
    wd->unnamed_pipe_next = INIT_WORKER_NEXT_PIPE; // à l'initialisation, le worker n'a pas de worker suivant
    wd->unnamed_pipe_master = unnamed_pipe_master;
    wd->worker_prime_number = worker_prime_number;
    wd->input_number = INIT_WORKER_VALUE;
    wd->hasNext = false;
}

// La fermeture d'un worker comprend:
// Fermeture des tubes annonymes
// Si le worker est le dernier de la chaine,
// Il envoie le message de succès au master.
void close_worker(worker_data *wd)
{
    close(wd->unnamed_pipe_previous);
    close(wd->unnamed_pipe_next);
    if (!wd->hasNext)
    {
        int toWrite = STOP_SUCCESS;
        int ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "worker - failed writing stop success to master\n");
        close(wd->unnamed_pipe_master);
    }
}

// Calcule si le nombre dont on veut tester la primalité a pour diviseur
// le nombre dont le worker a la charge (worker_prime_number)
// true -> input_number n'a pas worker_prime_number en diviseur, et est donc potentiellement lui meme premier
// false -> input_number a worker_prime_number comme diviseur, et donc input_number n'est pas premier
bool isPrime(worker_data *wd)
{
    return !(wd->input_number % wd->worker_prime_number == 0);
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
// Boucle principale du worker
void loop(worker_data *wd)
{
    bool cont = true;

    while (cont)
    {
        // Attente du prochain nombre à vérifier (ou de l'ordre d'arret)
        // provenant du worker précédent.
        // Note: pour le premier worker, l'ordre provient directement du
        // master.
        // La lecture du tube est bloquante, donc le worker attend ici
        // jusqu'à rećevoir une nouvelle tache
        int read_number;
        int ret = read(wd->unnamed_pipe_previous, &read_number, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "worker - reading order failed\n");

        // Le résultat de la lecture est soit:
        // - l'ordre d'arret "ORDRE_ARRET" qui arrete le worker et lance l'arret du suivant
        // - un entier dont il faut étudier la primalité pour le nombre du worker.
        //   Il faut ensuite soit avertir le master que le nombre est ou non premier,
        //   ou alors envoyer le nombre au worker suivant.
        //   Note: si le worker doit envoyer le nombre au worker suivant mais que
        //         le worker suivant n'existe pas, le worker se fork pour créer le suivant.

        if (read_number == ORDRE_ARRET)
        {
            // On arrête le worker et s'il possède un suivant, on lui envoie le signal d'arrêt
            if (wd->hasNext)
            {
                int toWrite = ORDRE_ARRET;
                ret = write(wd->unnamed_pipe_next, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing stop order to next worker\n");
            }
            close_worker(wd);
            cont = false;
            break;
        }
        // Pour compter le nombre "howmany", on compte le nombre de worker actuellement en marche.
        // Pour cela, le worker lit sur le tube l'entier du dernier, l'incrémente (se compte), et
        // envoie au worker suivant l'ordre et le nombre.
        // Dans le cas ou le worker est le dernier de la chaine, il retourne directement le nombre
        // au master
        else if (read_number == HOWMANY)
        {
            // Lecture du nombre précédent
            int read_number;
            int ret = read(wd->unnamed_pipe_previous, &read_number, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "worker - reading howmany count failed\n");
            if (wd->hasNext)
            {
                // Écriture au worker suivant l'ordre et le nombre
                int toWrite = HOWMANY;
                ret = write(wd->unnamed_pipe_next, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing order to next worker\n");
                toWrite = read_number + 1;
                ret = write(wd->unnamed_pipe_next, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing howmany to next worker\n");
            }
            else
            {
                // Écriture au master le compte total de worker
                int toWrite = read_number + 1;
                ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing howmany to master\n");
            }
        }
        // Pour déterminer le plus grand nombre premier calculer, il suffie de demander
        // au dernier worker de la chaine le nombre premier dont il est à la charge.
        // Pour cela, on envoie l'ordre de demande au worker suivant jusqu'au dernier
        // worker. Enfin, le dernier worker écrit au master le nombre premier dont il
        // a la charge
        else if (read_number == HIGHEST)
        {
            // Cas ou le worker est le dernier
            if (wd->hasNext)
            {
                // Écriture au master du nombre premier dont il est à la charge
                int toWrite = HIGHEST;
                ret = write(wd->unnamed_pipe_next, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing howmany to next worker\n");
            }
            // Cas du worker lambda
            else
            {
                // Écriture de l'ordre au worker suivant
                ret = write(wd->unnamed_pipe_master, &wd->worker_prime_number, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing howmany result to master\n");
            }
        }
        // Cas du calcul du nombre premier
        // Le nombre recu par le worker précédent (ou le master)
        // est contenu dans la variable read_number (qui est ajouté aux donnés du worker)
        else
        {
            wd->input_number = read_number;
            // Dans le cas ou le nombre à tester est égal au nombre premier dont le worker est
            // responsable, on a prouvé que le nombre est premier. On l'indique alors au master
            if (wd->input_number == wd->worker_prime_number)
            {
                int toWrite = IS_PRIME;
                ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing to master\n");
            }
            // Dans le cas ou le test de primalité pour le nombre du worker par rapport à l'input
            // montre que le nombre input ne peut pas être premier (parce qu'il possède le nombre
            // premier du worker comme diviseur), alors on l'indique au master
            else if (!isPrime(wd))
            {
                int toWrite = IS_NOT_PRIME;
                ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "worker - failed writing to master\n");
            }
            // Si le test de primalité est passé, alors on passe le nombre a tester
            // au worker suivant.
            // Note: si le worker suivant n'existe pas, alors il faut le créer via
            // un fork.
            else
            {
                // Cas ou le worker a un suivant.
                // On donne simplement le nombre au worker suivant.
                if (wd->hasNext)
                {
                    ret = write(wd->unnamed_pipe_next, &wd->input_number, sizeof(int));
                    CHECK_RETURN(ret == RET_ERROR, "worker - failed writing to next worker\n");
                }
                // Cas ou le worker n'a pas de suivant
                // On fork le worker actuel.
                // Il faut aussi créer un tube annonyme entre ce worker et le résultant
                // du fork.
                else
                {
                    int fds[2];
                    pipe(fds);
                    int resFork = fork();
                    CHECK_RETURN(resFork == RET_ERROR, "worker - failed fork to next worker\n");
                    if (resFork == 0) // worker suivant
                    {
                        // Mise à jour des tubes annonymes entre les deux workers
                        wd->worker_prime_number = wd->input_number;
                        wd->hasNext = false;
                        close(fds[WRITING]);
                        wd->unnamed_pipe_previous = fds[READING];
                        wd->unnamed_pipe_next = INIT_WORKER_NEXT_PIPE;
                        printf("Worker [%d] created\n", wd->worker_prime_number);
                        int toWrite = IS_PRIME;
                        ret = write(wd->unnamed_pipe_master, &toWrite, sizeof(int));
                        CHECK_RETURN(ret == RET_ERROR, "worker - failed writing to master\n");
                    }
                    else
                    {
                        // Mise à jour des tubes annonymes avec le worker suivant que
                        // l'on vient de créer
                        close(fds[READING]);
                        wd->hasNext = true;
                        wd->unnamed_pipe_next = fds[WRITING];
                    }
                }
            }
        }
    }
}

/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char *argv[])
{
    // Initialisation des donńees du premier worker
    worker_data wd;
    parseArgs(argc, argv, &wd);

    loop(&wd);

    return EXIT_SUCCESS;
}
