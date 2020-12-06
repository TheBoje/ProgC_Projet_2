/************************************************************************
 * Projet Numéro 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

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

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct master_data
{
    int mutex_clients_id;
    int mutex_client_master_id;
    int named_pipe_output;
    int named_pipe_input;
    int unnamed_pipe_output[2];
    int unnamed_pipe_inputs[2];
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

// Initialise les pipes entre le master et le client
void init_named_pipes()
{
    int ret1 = mkfifo(PIPE_MASTER_INPUT, 0641);
    int ret2 = mkfifo(PIPE_MASTER_OUTPUT, 0641);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "init_pipes - named pipes doesn't created\n");
}

// Initialise les pites entre le master et les workers
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

    return md;
}

// Envois n - 2 nombres aux workers pour calculer les nombres premiers
bool compute_prime(int n, master_data *md)
{
    int ret, read_result;

    // On envois les nombres de 2 à n - 1
    for (int i = 2; i < n; i++)
    {
        int toWrite = i;
        ret = write(md->unnamed_pipe_output[WRITING], &toWrite, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "master - failed write to worker\n");
        read_result = 0;
        ret = read(md->unnamed_pipe_inputs[READING], &read_result, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "master - failed read from worker\n");
    }

    // envois du nombre n
    ret = write(md->unnamed_pipe_output[WRITING], &n, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "compute_prime - writing n failed\n");

    // on récupère la sortie des worker (Si le nombre est premier ou non)
    int isPrime;
    ret = read(md->unnamed_pipe_inputs[READING], &isPrime, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "compute_prime - reading prime value failed\n");

    return isPrime == IS_PRIME;
}

// Retourne le nombre de nombres premiers on été calculés en tout
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

// Retourne le plus grand nombre premier calculé
int get_highest_prime(master_data *md)
{
    int highest = HIGHEST;
    int ret = write(md->unnamed_pipe_output[WRITING], &highest, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "get_highest_prime - writing order failed\n");

    ret = read(md->unnamed_pipe_inputs[READING], &highest, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "get_primes_numbers_calculated - reading nb failed\n");

    return highest;
}

// Détruit les sémaphores
void destroy_structure_sems(master_data *md)
{
    int ret1 = semctl(md->mutex_client_master_id, 0, IPC_RMID);
    int ret2 = semctl(md->mutex_clients_id, 0, IPC_RMID);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "destroy_structure_sems - failed destroy mutex\n");
}

// Détruit les sémaphores et les pipes entre le client et le master
void destroy_structure_pipes_sems(master_data *md)
{
    int ret1 = close(md->named_pipe_input);
    int ret2 = close(md->named_pipe_output);
    CHECK_RETURN(ret1 == RET_ERROR, "destroy_structure_pipes_sems - failed closing named pipes input\n");
    CHECK_RETURN(ret2 == RET_ERROR, "destroy_structure_pipes_sems - failed closing named pipes output\n")

    destroy_pipe(PIPE_MASTER_INPUT);
    destroy_pipe(PIPE_MASTER_OUTPUT);

    destroy_structure_sems(md);
}

// Ouvre les pipes entre le master et le client
void open_named_pipes_master(master_data *md)
{
    int fdsNamed[2];
    open_pipe(SIDE_MASTER, fdsNamed);
    md->named_pipe_input = fdsNamed[READING];
    md->named_pipe_output = fdsNamed[WRITING];
}

// Instruction envoyant l'ordre de fin au workers et stoppant le master
void stop(master_data *md)
{
    // Lancer l'odre de fin pour les worker
    int val = ORDRE_ARRET;
    int ret = write(md->unnamed_pipe_output[WRITING], &val, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "stop - write failed\n");

    // Attend le retour de fin des workers
    ret = read(md->unnamed_pipe_inputs[READING], &val, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR || val != STOP_SUCCESS, "stop - write failed or error closing workers\n");

    printf("Workers closed\n");

    close_pipes_master(md->unnamed_pipe_inputs, md->unnamed_pipe_output);

    // Envoie la confirmation de fin au client
    int confirmation = CONFIRMATION_STOP;
    ret = write(md->named_pipe_output, &confirmation, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "stop - write failed\n");
    printf("Master closed\n");
}

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/

// récupère le nombre à étudier et le retour des workers
void order_compute_prime(master_data *md)
{
    int n, ret;
    ret = read(md->named_pipe_input, &n, sizeof(int));
    CHECK_RETURN(ret == RET_ERROR, "loop - failed to read n\n");

    bool isPrime = compute_prime(n, md);
    ret = write(md->named_pipe_output, &isPrime, sizeof(bool));
    CHECK_RETURN(ret == RET_ERROR, "loop - failed to write to client\n");
}

// Attennd l'ordre du client et traite l'information
void loop(master_data *md)
{
    bool cont = true;

    while (cont)
    {
        printf("Master is waiting for order\n");
        open_named_pipes_master(md); // Ouverture des pipes en liaison avec les clients (l'ouverture est blocante)

        int order;
        int ret = read(md->named_pipe_input, &order, sizeof(int));
        printf("Order [%d]\n", order);
        // Si on est créé c'est qu'on est un nombre premier
        // Envoyer au master un message positif pour dire
        // que le nombre testé est bien premier
        CHECK_RETURN(ret == RET_ERROR, "loop - failed to read order\n");

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
            CHECK_RETURN(ret == RET_ERROR, "loop - failed to write to client \n");

            break;
        }

        case ORDER_HIGHEST_PRIME:
        {
            int highest = get_highest_prime(md);
            ret = write(md->named_pipe_output, &highest, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "loop - failed to write to client\n");
            break;
        }

        default:
        {
            TRACE("master - loop - wrong order from client\n");
            exit(EXIT_FAILURE);
            break;
        }
        }

        // On prend le mutex pour être sur de fermer les pipes correctement si on continue la boucle (permet d'attendre le client aussi)
        take_mutex(md->mutex_client_master_id);
        if (cont)
        {
            ret = close(md->named_pipe_input);
            CHECK_RETURN(ret == RET_ERROR, "loop - failed to close named pipes input\n");

            ret = close(md->named_pipe_output);

            CHECK_RETURN(ret == RET_ERROR, "loop - failed to close named pipes output\n");
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
    printf("Master initialised successfully\n");
    // boucle infinie
    loop(&md);

    destroy_structure_pipes_sems(&md); // destruction des tubes nommés, des sémaphores, ...

    return EXIT_SUCCESS;
}