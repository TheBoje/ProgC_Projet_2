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

bool compute_prime_local(int input);

/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char *argv[])
{
    int number = 0;
    int order = parseArgs(argc, argv, &number);
    printf("%d\n", order); // pour éviter le warning

    // order peut valoir 5 valeurs (cf. master_client.h) :
    //      - ORDER_COMPUTE_PRIME_LOCAL
    //      - ORDER_STOP
    //      - ORDER_COMPUTE_PRIME
    //      - ORDER_HOW_MANY_PRIME
    //      - ORDER_HIGHEST_PRIME
    //
    // si c'est ORDER_COMPUTE_PRIME_LOCAL
    //    alors c'est un code complètement à part multi-thread
    //      -> lancer compute_prime_local(number)
    // sinon
    //    - entrer en section critique : (prendre le mutex dans master_client)
    //           . pour empêcher que 2 clients communiquent simultanément
    //           . le mutex est déjà créé par le master
    //    - ouvrir les tubes nommés (ils sont déjà créés par le master) dans master_client
    //           . les ouvertures sont bloquantes, il faut s'assurer que
    //             le master ouvre les tubes dans le même ordre
    //    - envoyer l'ordre et les données éventuelles au master
    //          -> écriture dans le pipe, et lecture pour le master (dans master_client)
    //    - attendre la réponse sur le second tube
    //          -> lire dans le pipe (dans master_client)
    //          -> prendre second mutex
    //    - sortir de la section critique
    //          -> vendre le mutex (dans master_client)
    //    - libérer les ressources (fermeture des tubes, ...)
    //          -> fermeture des tubes (dans master_client)
    //    - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
    //          -> vendre second mutex ORDER_STOP
    //
    // Une fois que le master a envoyé la réponse au client, il se bloque
    // sur un sémaphore ; le dernier point permet donc au master de continuer
    if (order == ORDER_COMPUTE_PRIME_LOCAL)
    {
        bool res = compute_prime_local(number);
        printf("Number : %d, result : %b", number, res);
    }
    else
    {
        int sem_clients_id = semget(ID_CLIENTS, 1, IPC_RMID);
        take_mutex(sem_clients_id);
        int fd[2] = {open(PIPE_CLIENT_OUTPUT, O_WRONLY), open(PIPE_CLIENT_INPUT, O_RDONLY)};

        open_pipe(fd, SIDE_CLIENT);
        // TODO verif write success
        write(fd[1], &order, sizeof(int));
        if (order == ORDER_COMPUTE_PRIME)
        {
            write(fd[1], &number, sizeof(int));
        }
        int result_read;
        read(fd[0], &result_read, sizeof(int));
        printf("Order : %d, Result : %d", order, result_read);
        int sem_master_client_id = semget(ID_MASTER_CLIENT, 0, 0);
        take_mutex(sem_master_client_id);
        sell_mutex(sem_clients_id);
        close_pipe(fd);
        sell_mutex(sem_master_client_id);
    }

    return EXIT_SUCCESS;
}
