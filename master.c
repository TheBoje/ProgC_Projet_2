#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"
// TODO Set me to 0
#define INIT_MASTER_VALUE 0

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
    int ret, read_result;

    for (int i = 2; i < n; i++)
    {
        // TODO
        // -> envois des nombres de 0 à n-1 au premier worker via le tube anonyme output
        // -> on traite les sorties des workrs via le tube anonyme input
        printf("Envoi du nombre %d\n", i);
        int toWrite = i;
        ret = write(md->unnamed_pipe_output[WRITING], &toWrite, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "master - failed write to worker\n");
        read_result = 0;
        ret = read(md->unnamed_pipe_inputs[READING], &read_result, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "master - failed read from worker\n");
        printf("RESULT WORKER [%d] | [%d]\n", i, read_result);
    }

    // -> envois du nombre n
    ret = write(md->unnamed_pipe_output[WRITING], &n, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "compute_prime - writing n failed\n");
    // -> on récupère la sortie des worker via le tube anonyme input
    int isPrime;
    ret = read(md->unnamed_pipe_inputs[READING], &isPrime, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "compute_prime - reading prime value failed\n");
    // -> n est premier - oui ou non
    return isPrime == IS_PRIME;
}

// How many prime - ORDER_HOW_MANY_PRIME
int get_primes_numbers_calculated(master_data *md)
{
    int howMany = HOWMANY;
    int ret = write(md->unnamed_pipe_output[WRITING], &howMany, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "get_primes_numbers_calculated - writing order failed\n");

    int nb = 0;
    ret = write(md->unnamed_pipe_output[WRITING], &nb, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "get_primes_numbers_calculated - writing nb failed\n");

    ret = read(md->unnamed_pipe_inputs[READING], &nb, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "get_primes_numbers_calculated - reading nb failed\n");

    return nb;
}

// Return ORDER_HIGHEST_PRIME
int get_highest_prime(master_data *md)
{
    return md->highest_prime;
}

void destroy_structure_sems(master_data *md)
{
    int ret1 = semctl(md->mutex_client_master_id, 0, IPC_RMID);
    int ret2 = semctl(md->mutex_clients_id, 0, IPC_RMID);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "destroy_structure_sems - failed destroy mutex\n");
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

    destroy_structure_sems(md);

    printf("Done destroying\n");
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
    wait(NULL);
    printf("Fin des workers\n");

    

    int confirmation = CONFIRMATION_STOP;
    int ret = write(md->named_pipe_output, &confirmation, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "stop - write failed\n");
    // destroy_structure_pipes_sems(md);
    printf("Fin du master\n");
}

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/

void order_compute_prime(master_data *md)
{
    int n, ret;
    ret = read(md->named_pipe_input, &n, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "loop - failed reading n\n");

    bool isPrime = compute_prime(n, md);
    ret = write(md->named_pipe_output, &isPrime, sizeof(bool));
    CHECK_RETURN(ret == RET_ERROR, "loop - failed writing is prime\n");
}

void loop(master_data *md)
{
    bool cont = true;

    while (cont)
    {
        printf("Attente de l'ouverture du pipe\n");
        open_named_pipes_master(md); // Ouverture des pipes en liaison avec les clients (l'ouverture est blocante)

        int order;
        int ret = read(md->named_pipe_input, &order, sizeof(int));
        printf("Valeur read : %d\n", order);
        CHECK_RETURN(ret == RET_ERROR, "loop - reading order failed\n");

        switch (order)
        {
        case ORDER_STOP:
            cont = false;
            stop(md);
            break;

        case ORDER_COMPUTE_PRIME:
            order_compute_prime(md);
            break;

        case ORDER_HOW_MANY_PRIME:
        {
            int howManyCalc = get_primes_numbers_calculated(md);
            ret = write(md->named_pipe_output, &howManyCalc, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "loop - failed writing how many prime\n");

            break;
        }

        case ORDER_HIGHEST_PRIME:
        {
            int highest = get_highest_prime(md);
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

        // On prend le mutex pour être sur de fermer les pipes correctement si on continue la boucle (permet d'attendre le client aussi)
        take_mutex(md->mutex_client_master_id);
        if (cont)
        {
            ret = close(md->named_pipe_input);
            CHECK_RETURN(ret == RET_ERROR, "loop - failed closing named pipes input\n");

            ret = close(md->named_pipe_output);

            CHECK_RETURN(ret == RET_ERROR, "loop - failed closing named pipes output\n");
        }
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

    init_named_pipes();                       // Initialisation des pipes pour les clients
    master_data md = init_master_structure(); // Initialisation des pipes pour les workers et des sémaphores

    // - création du premier worker
    //      -> init premier worker
    create_pipes_master(md.unnamed_pipe_inputs, md.unnamed_pipe_output);
    create_worker(md.unnamed_pipe_output[READING], md.unnamed_pipe_inputs[WRITING]);
    init_pipes_master(md.unnamed_pipe_inputs, md.unnamed_pipe_output);

    // boucle infinie
    loop(&md);

    destroy_structure_pipes_sems(&md); // destruction des tubes nommés, des sémaphores, ...

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
