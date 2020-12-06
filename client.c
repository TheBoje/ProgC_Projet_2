#include "config.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "myassert.h"

#include "master_client.h"

// chaines possibles pour le premier paramètre de la ligne de commande
#define TK_STOP "stop"
#define TK_COMPUTE "compute"
#define TK_HOW_MANY "howmany"
#define TK_HIGHEST "highest"
#define TK_LOCAL "local"

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <ordre> [<number>]\n", exeName);
    fprintf(stderr, "   ordre \"" TK_STOP "\" : arrêt master\n");
    fprintf(stderr, "   ordre \"" TK_COMPUTE "\" : calcul de nombre premier\n");
    fprintf(stderr, "                       <nombre> doit être fourni\n");
    fprintf(stderr, "   ordre \"" TK_HOW_MANY "\" : combien de nombres premiers calculés\n");
    fprintf(stderr, "   ordre \"" TK_HIGHEST "\" : quel est le plus grand nombre premier calculé\n");
    fprintf(stderr, "   ordre \"" TK_LOCAL "\" : calcul de nombre premier en local\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static int parseArgs(int argc, char *argv[], int *number)
{
    int order = ORDER_NONE;

    if ((argc != 2) && (argc != 3))
        usage(argv[0], "Nombre d'arguments incorrect");

    if (strcmp(argv[1], TK_STOP) == 0)
        order = ORDER_STOP;
    else if (strcmp(argv[1], TK_COMPUTE) == 0)
        order = ORDER_COMPUTE_PRIME;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        order = ORDER_HOW_MANY_PRIME;
    else if (strcmp(argv[1], TK_HIGHEST) == 0)
        order = ORDER_HIGHEST_PRIME;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        order = ORDER_COMPUTE_PRIME_LOCAL;

    if (order == ORDER_NONE)
        usage(argv[0], "ordre incorrect");
    if ((order == ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas de second argument");
    if ((order == ORDER_COMPUTE_PRIME) && (argc != 3))
        usage(argv[0], TK_COMPUTE " : il faut le second argument");
    if ((order == ORDER_HOW_MANY_PRIME) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas de second argument");
    if ((order == ORDER_HIGHEST_PRIME) && (argc != 2))
        usage(argv[0], TK_HIGHEST " : il ne faut pas de second argument");
    if ((order == ORDER_COMPUTE_PRIME_LOCAL) && (argc != 3))
        usage(argv[0], TK_LOCAL " : il faut le second argument");
    if ((order == ORDER_COMPUTE_PRIME) || (order == ORDER_COMPUTE_PRIME_LOCAL))
    {
        *number = strtol(argv[2], NULL, 10);
        if (*number < 2)
            usage(argv[0], "le nombre doit être >= 2");
    }

    return order;
}

/************************************************************************
 * Fonctions secondaires
 ************************************************************************/

typedef struct
{
    int valeur;
    int target;
    bool *isPrime;
} ThreadData;

void *threadPrime(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    if (data->target % data->valeur == 0)
    {
        *(data->isPrime) = false;
    }
    else
    {
        *(data->isPrime) = true;
    }
    return NULL;
}

bool compute_prime_local(int input)
{
    fprintf(stdout, "Computing prime local of number [%d]\n", input);

    int size = sqrt(input) + 1;
    bool isPrimeTab[size];
    ThreadData datas[size];
    pthread_t threads[size];

    for (int i = 0; i < size; i++)
    {
        isPrimeTab[i] = true;
        datas[i].valeur = i;
        datas[i].target = input;
        datas[i].isPrime = &isPrimeTab[i];
        *datas[i].isPrime = true;
    }

    for (int i = 2; i < size; i++)
    {
        int ret = pthread_create(&(threads[i]), NULL, threadPrime, &datas[i]);
        CHECK_RETURN(ret == RET_ERROR, "Client local compute - Error threads create\n");
    }

    for (int i = 2; i < size; i++)
    {
        int ret = pthread_join(threads[i], NULL);
        CHECK_RETURN(ret == RET_ERROR, "Client local compute - Error threads join\n");
    }

    bool res = true;
    for (int i = 0; i < size; i++)
    {
        if (isPrimeTab[i] == false)
        {
            res = false;
            break;
        }
    }

    return res;
}

/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char *argv[])
{
    int number = 0;
    int order = parseArgs(argc, argv, &number);

    /* =======================================================
    == order peut valoir 5 valeurs (cf. master_client.h) :  ==
    ==      - ORDER_COMPUTE_PRIME_LOCAL                     ==
    ==      - ORDER_STOP                                    ==
    ==      - ORDER_COMPUTE_PRIME                           ==
    ==      - ORDER_HOW_MANY_PRIME                          ==
    ==      - ORDER_HIGHEST_PRIME                           ==
    ======================================================= */

    // si c'est ORDER_COMPUTE_PRIME_LOCAL
    //    alors c'est un code complètement à part multi-thread
    if (order == ORDER_COMPUTE_PRIME_LOCAL)
    {
        bool res = compute_prime_local(number);
        printf("Number : [%d] | Is prime : [%s]\n", number, res ? "true" : "false");
    }
    else
    {
        //  - entrer en section critique : (prendre le mutex dans master_client)
        //      . pour empêcher que 2 clients communiquent simultanément
        //      . le mutex est déjà créé par le master
        int sem_clients_id = semget(ftok(FILE_KEY, ID_CLIENTS), 1, 0);
        int sem_master_client_id = semget(ftok(FILE_KEY, ID_MASTER_CLIENT), 1, 0);

        take_mutex(sem_master_client_id);
        take_mutex(sem_clients_id);

        //  - ouvrir les tubes nommés (ils sont déjà créés par le master) dans master_client
        //    . TODO les ouvertures sont bloquantes, il faut s'assurer que
        //      le master ouvre les tubes dans le même ordre
        int fd[2];
        open_pipe(SIDE_CLIENT, fd);

        //  - envoyer l'ordre et les données éventuelles au master
        int ret = write(fd[WRITING], &order, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "Client - Error write order to master\n");

        // Dans le cas ou on demande de calculer si le nombre est premier
        // On envoie dans un second temps le nombre premier à vérifier

        if (order == ORDER_COMPUTE_PRIME)
        {
            write(fd[1], &number, sizeof(int));
            bool result_read;
            ret = read(fd[READING], &result_read, sizeof(bool));
            CHECK_RETURN(ret == RET_ERROR, "Client - Error read from master\n");
            printf("Order : [%d] | Number : [%d] | Is Prime : [%s]\n", order, number, result_read ? "true" : "false");
        }
        else if (order == ORDER_STOP)
        {
            int result_read;
            ret = read(fd[READING], &result_read, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR || result_read != CONFIRMATION_STOP, "Client - Error read from master\n");

            printf("Master and worker(s) stopped successfully\n");
        }
        else
        {
            int result_read;
            ret = read(fd[READING], &result_read, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "Client - Error read from master\n");
            printf("Order : [%d] | Result : [%d]\n", order, result_read);
        }

        //  - attendre la réponse sur le second tube
        //      -> lire dans le pipe

        //      -> prendre second mutex
        //take_mutex(sem_master_client_id);

        //  - sortir de la section critique
        //      -> vendre le mutex

        sell_mutex(sem_clients_id);
        //  - libérer les ressources (fermeture des tubes, ...)
        //      -> fermeture des tubes
        //close_pipe(fd);

        ret = close(fd[READING]);
        CHECK_RETURN(ret == RET_ERROR, "Client - Error closing pipe reading\n");

        //  - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        //      -> vendre second mutex ORDER_STOP

        ret = close(fd[WRITING]);
        CHECK_RETURN(ret == RET_ERROR, "Client - Error closing pipe writting\n");

        sell_mutex(sem_master_client_id);
    }
    return EXIT_SUCCESS;
}
