#if defined HAVE_CONFIG_H
#include "config.h"
#endif

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

    bool *res = malloc(sizeof(bool));
    *res = true;
    if (data->target % data->valeur == 0)
    {
        *res = false;
    }
    else
    {
        *res = true;
    }
    *(((ThreadData *)arg)->isPrime) = *res;
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
        myassert(ret == 0, "Error threads create\n");
    }

    for (int i = 2; i < size; i++)
    {
        int ret = pthread_join(threads[i], NULL);
        myassert(ret == 0, "Error threads join\n");
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
    printf("%d\n", order); // pour éviter le warning

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
        printf("Number : [%d] | result : [%s]\n", number, res ? "true" : "false");
    }
    else
    {
        //  - entrer en section critique : (prendre le mutex dans master_client)
        //      . pour empêcher que 2 clients communiquent simultanément
        //      . le mutex est déjà créé par le master
        int sem_clients_id = semget(ftok(FILE_KEY, ID_CLIENTS), 1, 0);
        take_mutex(sem_clients_id);

        //  - ouvrir les tubes nommés (ils sont déjà créés par le master) dans master_client
        //    . TODO les ouvertures sont bloquantes, il faut s'assurer que
        //      le master ouvre les tubes dans le même ordre
        int fd[2];
        open_pipe(SIDE_CLIENT, fd);

        //  - envoyer l'ordre et les données éventuelles au master
        int res_write = write(fd[WRITING], &order, sizeof(int));
        if (res_write == RET_ERROR)
        {
            fprintf(stderr, "Error write order to master\n");
            exit(EXIT_FAILURE);
        }
        // Dans le cas ou on demande de calculer si le nombre est premier
        // On envoie dans un second temps le nombre premier à vérifier
        if (order == ORDER_COMPUTE_PRIME)
        {
            write(fd[1], &number, sizeof(int));
        }

        //  - attendre la réponse sur le second tube
        //      -> lire dans le pipe
        int result_read;
        int res = read(fd[READING], &result_read, sizeof(int));
        if (res == RET_ERROR)
        {
            fprintf(stderr, "Error read order from master\n");
            exit(EXIT_FAILURE);
        }
        printf("Order : [%d] | Result : [%d]\n", order, result_read);

        //      -> prendre second mutex
        int sem_master_client_id = semget(ftok(FILE_KEY, ID_MASTER_CLIENT), 0, 0);
        take_mutex(sem_master_client_id);

        //  - sortir de la section critique
        //      -> vendre le mutex
        sell_mutex(sem_clients_id);
        //  - libérer les ressources (fermeture des tubes, ...)
        //      -> fermeture des tubes
        close_pipe(fd);
        //  - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        //      -> vendre second mutex ORDER_STOP
        sell_mutex(sem_master_client_id);

        // TODO Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
    }
    return EXIT_SUCCESS;
}
