//#define HAVE_CONFIG_H

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
// TODO Set me to 0
#define INIT_MASTER_VALUE 1

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

// Création et initialisation des sémaphores
void init_sem(int *sem_client_id, int *sem_client_master_id)
{
    int sem1 = semget(ftok(FILE_KEY, ID_CLIENTS), 1, IPC_CREAT | IPC_EXCL | 0641);
    int sem2 = semget(ftok(FILE_KEY, ID_MASTER_CLIENT), 1, IPC_CREAT | IPC_EXCL | 0641);
    CHECK_RETURN(sem1 == RET_ERROR || sem2 == RET_ERROR, "init_sem - semaphore doesn't created\n");

    int ret1 = semctl(sem1, 0, SETVAL, 1);
    int ret2 = semctl(sem2, 0, SETVAL, 1);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "init_sem - semaphore doesn't initialize\n");

    *sem_client_id = sem1;
    *sem_client_master_id = sem2;
}

void init_named_pipes()
{
    int ret1 = mkfifo(PIPE_MASTER_INPUT, 0641);
    int ret2 = mkfifo(PIPE_MASTER_OUTPUT, 0641);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "init_pipes - named pipes doesn't created\n");
}

void init_workers_pipes(int unnamed_pipe_input[], int unnamed_pipe_output[])
{
    int ret1 = close(unnamed_pipe_input[WRITING]);
    int ret2 = close(unnamed_pipe_output[READING]);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "init_workers_pipes - closing pipes failed\n");
}

// Initialise la structure de donnée
master_data init_master_structure()
{
    master_data md;

    // Initialise les sémaphores et les tubes nommés (voir master_client.c)
    init_sem(&(md.mutex_clients_id), &(md.mutex_client_master_id));

    // Initialise le tube anonyme pour la lecture des renvois des workers
    int ret = pipe(md.unnamed_pipe_output);
    CHECK_RETURN(ret == RET_ERROR, "init_master_structure - anonymous output pipe doesn't created\n");

    // Initialise le tube anonyme pour l'écriture vers les workers
    ret = pipe(md.unnamed_pipe_inputs);
    CHECK_RETURN(ret == RET_ERROR, "init_master_structure - anonymous input pipe doesn't created\n");

    init_workers_pipes(md.unnamed_pipe_inputs, md.unnamed_pipe_output);

    md.primes_number_calculated = INIT_MASTER_VALUE;
    md.highest_prime = INIT_MASTER_VALUE;

    return md;
}

// Compute prime - ORDER_COMPUTE_PRIME (N)
bool compute_prime(int n, master_data *md)
{
    for (int i = 2; i < n; i++)
    {
        // -> envois des nombres de 0 à n-1 au premier worker via le tube anonyme output
        // -> on traite les sorties des workrs via le tube anonyme input
        printf("Envois du nombre %d\n", i);
    }

    // -> envois du nombre n
    // -> on récupère la sortie des worker via le tube anonyme input
    // -> n est premier - oui ou non
    return true;
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

void destroy_structure_pipes_sems(master_data *md)
{
    printf("Destroying pipes\n");
    int ret1 = close(md->named_pipe_input);
    int ret2 = close(md->named_pipe_output);
    CHECK_RETURN(ret1 == RET_ERROR, "destroy_structure_pipes_sems - failed closing named pipes input\n");
    CHECK_RETURN(ret2 == RET_ERROR, "destroy_structure_pipes_sems - failed closing named pipes output\n")

    destroy_pipe(PIPE_MASTER_INPUT);
    destroy_pipe(PIPE_MASTER_OUTPUT);

    printf("Done destroying\n");
}

void destroy_structure_sems(master_data *md)
{
    int ret1 = semctl(md->mutex_client_master_id, 0, IPC_RMID);
    int ret2 = semctl(md->mutex_clients_id, 0, IPC_RMID);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "destroy_structure_sems - failed destroy mutex\n");
}

void open_named_pipes_master(master_data *md)
{
    int fdsNamed[2];
    open_pipe(SIDE_MASTER, fdsNamed);
    md->named_pipe_input = fdsNamed[READING];
    md->named_pipe_output = fdsNamed[WRITING];
}

// Envoie d'accusé de reception - ORDER_STOP TODO
void stop(master_data *md)
{
    // -> Lancer l'odre de fin pour les worker
    // -> attendre la fin des workers
    // -> envoyer le signal de fin au client
    // TODO Delete sémaphores et tubes
    // TODO attendre la fin des workers
    printf("Fin des workers\n");
    int confirmation = CONFIRMATION_STOP;
    int ret = write(md->named_pipe_output, &confirmation, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "stop - write failed\n");
    destroy_structure_pipes_sems(md);
    printf("Fin du master\n");
}

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/

// TODO Mettre sémaphore ici
// - si ORDER_STOP
//       . envoyer ordre de fin au premier worker et attendre sa fin (dans master_worker)
//       . envoyer un accusé de réception au client
// - si ORDER_COMPUTE_PRIME
//       . récupérer le nombre N à tester provenant du client (dans master_client)
//       . construire le pipeline jusqu'au nombre N-1 (si non encore fait) :
//             il faut connaître le plus grand nombre (M) déjà envoyé aux workers
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
// TODO Répondre : Sinon 2 clients peuvent écrire et lire en même temps

void loop(master_data *md)
{
    bool cont = true;
    while (cont)
    {
        // boucle infinie :
        // - ouverture des tubes (cf. rq client.c) (dans master_client)
        printf("Attente de l'ouverture du pipe\n");
        open_named_pipes_master(md);
        // - attente d'un ordre du client (via le tube nommé) (dans master_client)
        int order;

        int ret = read(md->named_pipe_input, &order, sizeof(int));
        printf("Valeur read : %d\n", order);
        CHECK_RETURN(ret == RET_ERROR, "loop - reading order failed\n");

        switch (order)
        {
            case ORDER_STOP:
            {
                cont = false;
                stop(md);
                break;
            }


            case ORDER_COMPUTE_PRIME:
            {
                int n;
                ret = read(md->named_pipe_input, &n, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "loop - failed reading n\n");

                bool isPrime = compute_prime(n, md);
                ret = write(md->named_pipe_output, &isPrime, sizeof(bool));
                CHECK_RETURN(ret == RET_ERROR, "loop - failed writing is prime\n");

                break;
            }

            case ORDER_HOW_MANY_PRIME:
            {
                int howManyCalc = get_primes_numbers_calculated(*md);
                printf("Préparation envois du nombre de nombres premiers\n");
                ret = write(md->named_pipe_output, &howManyCalc, sizeof(int));
                printf("Envois du nombre de nombres premiers\n");
                CHECK_RETURN(ret == RET_ERROR, "loop - failed writing how many prime\n");

                break;
            }

            case ORDER_HIGHEST_PRIME:
            {
                int highest = get_highest_prime(*md);
                ret = write(md->named_pipe_output, &highest, sizeof(int));
                CHECK_RETURN(ret == RET_ERROR, "loop - failed writing highest prime\n");

                break;
            }

            default:
            {
                TRACE("Order failure\n");
                exit(EXIT_FAILURE);
                break;
            }

        }
        int what;
        read(md->named_pipe_input, &what, sizeof(int));
        printf("Hey buddy c'est moi Chiantos le chiant %d\n", what);
        take_mutex(md->mutex_client_master_id);
        int ret1 = close(md->named_pipe_input);


        int ret2 = close(md->named_pipe_output);
        CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "destroy_structure_pipes_sems - failed closing named pipes\n");
        sell_mutex(md->mutex_client_master_id);
    }
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
    init_named_pipes();
    master_data md = init_master_structure();
    // - création du premier worker
    //      -> init premier worker

    // boucle infinie
    loop(&md);

    // destruction des tubes nommés, des sémaphores, ...
    destroy_structure_sems(&md);

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
