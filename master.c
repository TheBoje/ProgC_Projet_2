#define HAVE_CONFIG_H

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"

#define INIT_MASTER_VALUE 0
#define WRITING 1
#define READING 0
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
typedef struct master_data
{
    int mutex_clients_id;
    int mutex_client_master_id;
    int named_pipe_output;
    int named_pipe_input;
    int unnamed_pipe_output[2];
    int unnamed_pipe_inputs[2];
    int primes_number_calculated;
    int highest_prime;
} master_data;
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

void init_sem(int *sem_client_id, int *sem_client_master_id)
{
    int sem1 = semget(ftok(FILE_KEY, ID_CLIENTS), 1, IPC_CREAT | IPC_EXCL | 0641);
    int sem2 = semget(ftok(FILE_KEY, ID_MASTER_CLIENT), 1, IPC_CREAT | IPC_EXCL | 0641);

    if (sem1 == RET_ERROR || sem2 == RET_ERROR)
    {
        TRACE("init_sem - semaphore doesn't created\n");
        exit(EXIT_FAILURE);
    }

    *sem_client_id = sem1;
    *sem_client_master_id = sem2;
}

void init_named_pipes(int *input_pipe_client, int *output_pipe_client)
{
    int ret1 = mkfifo(PIPE_MASTER_INPUT, 0641);
    int ret2 = mkfifo(PIPE_MASTER_OUTPUT, 0641);

    if (ret1 == RET_ERROR || ret2 == RET_ERROR)
    {
        TRACE("init_pipes - named pipes doesn't created\n");
        exit(EXIT_FAILURE);
    }
}

void init_workers_pipes(int unnamed_pipe_input[], int unnamed_pipe_output[])
{
    int ret1 = close(unnamed_pipe_input[WRITING]);
    int ret2 = close(unnamed_pipe_output[READING]);

    if (ret1 == RET_ERROR || ret2 == RET_ERROR)
    {
        TRACE("init_workers_pipes - closing pipes failed");
        exit(EXIT_FAILURE);
    }
}

// Initialise la structure de donnée
master_data init_master_structure()
{
    master_data md;

    // Initialise les sémaphores et les tubes nommés (voir master_client.c)
    init_sem(&(md.mutex_clients_id), &(md.mutex_client_master_id));
    init_named_pipes(&(md.named_pipe_input), &(md.named_pipe_output));

    // Initialise le tube anonyme pour la lecture des renvois des workers
    int ret = pipe(md.unnamed_pipe_output);
    if (ret == RET_ERROR)
    {
        TRACE("init_master_structure - anonymous output pipe doesn't created");
        exit(EXIT_FAILURE);
    }

    // Initialise le tube anonyme pour l'écriture vers les workers
    ret = pipe(md.unnamed_pipe_inputs);
    if (ret == RET_ERROR)
    {
        TRACE("init_master_structure - anonymous input pipe doesn't created")
    }

    init_workers_pipes(&(md.unnamed_pipe_inputs), &(md.unnamed_pipe_output));

    md.primes_number_calculated = INIT_MASTER_VALUE;
    md.highest_prime = INIT_MASTER_VALUE;

    return md;
}

// Envoie d'accusé de reception - ORDER_STOP TODO
void stop()
{
    // -> Lancer l'odre de fin pour les worker
    // -> attendre la fin des workers
    // -> envoyer le signal de fin au client

    printf("Fin des workers\n");
    printf("Fin du master\n");
}

// Compute prime - ORDER_COMPUTE_PRIME (N)
int compute_prime(int n)
{
    for (int i = 2; i < n; i++)
    {
        // -> envois des nombres de 0 à n-1 au premier worker via le tube anonyme output
        // -> on traite les sorties des workrs via le tube anonyme input
        printf("Envois du nombre %d", i);
    }

    // -> envois du nombre n
    // -> on récupère la sortie des worker via le tube anonyme input
    // -> n est premier - oui ou non
    return EXIT_SUCCESS;
}

// How many prime - ORDER_HOW_MANY_PRIME
int get_primes_numbers_calculated(master_data md)
{
    return md.primes_number_calculated;
}

// Return ORDER_HIGHEST_PRIME
int get_highest_prime(master_data md)
{
    return md.highest_prime;
}

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
