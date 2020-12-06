/************************************************************************
 * Projet Num 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

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
// Définition de la structure de donnée transmise à chaque thread
// Note : *isPrime est le pointeur vers la case que le thread doit remplir
typedef struct
{
    int valeur;
    int target;
    bool *isPrime;
} ThreadData;

// Fonction lancée pour chaque thread.
// Chaque thread doit remplir la case *isPrime dans le tableau contenu dans compute_prime_local
// Écrit false si le nombre n'est pas premier, et true s'il l'est.
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

// Calcul d'un nombre premier en local (appel via ./client local <nb>)
// Lance sqrt(nb) + 1 threads pour vérifier si au moins 1 des sqrt(nb) + 1
// premier nombre est un diviseur de nb.
// Chaque thread est lancé via threadPrime.
// Note: le nombre d'appel de threads est limité par la machine.
// Voir cat /proc/sys/kernel/threads-max pour le nombre de Threads max pour votre machine.
// Note2: le nombre est aussi limité par MAX_INT étant donné la structure de données actuelle.
bool compute_prime_local(int input)
{
    fprintf(stdout, "Computing prime local of number [%d]\n", input);

    int size = sqrt(input) + 1;
    bool isPrimeTab[size];
    ThreadData datas[size];
    pthread_t threads[size];
    // Initialisation de la structure de données
    for (int i = 0; i < size; i++)
    {
        isPrimeTab[i] = true;
        datas[i].valeur = i;
        datas[i].target = input;
        datas[i].isPrime = &isPrimeTab[i];
        *datas[i].isPrime = true; // On remplie le tableau de booléen par true
    }
    // Lancement des threads
    for (int i = 2; i < size; i++)
    {
        int ret = pthread_create(&(threads[i]), NULL, threadPrime, &datas[i]);
        CHECK_RETURN(ret == RET_ERROR, "Client local compute - Error threads create\n");
    }
    // Attente de la fin de tous les threads
    for (int i = 2; i < size; i++)
    {
        int ret = pthread_join(threads[i], NULL);
        CHECK_RETURN(ret == RET_ERROR, "Client local compute - Error threads join\n");
    }
    // Parcours du tableau (initialisé à true et modifié par chaque thread lancé)
    // Si le nombre du thread est un diviseur du nombre cible, alors le thread
    // remplace son booléen par false. Si au moins une des cases est à false,
    // alors le nombre n'est pas premier.
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

    // ORDER_COMPUTE_PRIME_LOCAL
    // alors c'est un code complètement à part multi-thread
    // Voir fonction bool compute_prime_local(int)
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

        // Initialisation des sémaphores
        // sem_clients_id empeche 2 clients de se connecter au master en même temps
        // sem_master_client_id permet au master d'attendre le client et inversement
        int sem_clients_id = semget(ftok(FILE_KEY, ID_CLIENTS), 1, 0);
        int sem_master_client_id = semget(ftok(FILE_KEY, ID_MASTER_CLIENT), 1, 0);

        // TODO IS THIS CORRECT ?
        take_mutex(sem_clients_id);
        take_mutex(sem_master_client_id);

        // Ouverture des tubes nommés avec le master
        int fd[2];
        open_pipe(SIDE_CLIENT, fd);

        // Envoie de l'ordre au master.
        // Les ordre sont représentés par des <int>
        // Voir master_client.h
        int ret = write(fd[WRITING], &order, sizeof(int));
        CHECK_RETURN(ret == RET_ERROR, "Client - Error write order to master\n");

        // Cas de l'ordre de demande de calcul du nombre <number>
        // On envoie alors au master le nombre a vérifier vie le tube.
        if (order == ORDER_COMPUTE_PRIME)
        {
            // Envoie du nombre au master
            ret = write(fd[1], &number, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "Client - Error write number to master\n");

            // Lecture du résultat
            // La lecture est bloquante, donc le client attends ici le résultat de master
            bool result_read;
            ret = read(fd[READING], &result_read, sizeof(bool));
            CHECK_RETURN(ret == RET_ERROR, "Client - Error read from master\n");
            printf("Order : [%d] | Number : [%d] | Is Prime : [%s]\n", order, number, result_read ? "true" : "false");
        }
        // Cas le l'ordre de demande d'arrêt au master (et aux workers)
        else if (order == ORDER_STOP)
        {
            // Attente de la confirmation d'arrêt du master (et donc indirectement aussi des workers)
            int result_read;
            ret = read(fd[READING], &result_read, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR || result_read != CONFIRMATION_STOP, "Client - Error read from master\n");

            printf("Master and worker(s) stopped successfully\n");
        }
        // Autres cas : Highest et Howmany
        // Le client attend la réponse du master
        else
        {
            // Lecture bloquante, le client attend ici jusqu'à réponse du master
            int result_read;
            ret = read(fd[READING], &result_read, sizeof(int));
            CHECK_RETURN(ret == RET_ERROR, "Client - Error read from master\n");
            printf("Order : [%d] | Result : [%d]\n", order, result_read);
        }
        // Fin de section critique
        sell_mutex(sem_master_client_id);
        // Fermeture des pipes avec le master
        ret = close(fd[READING]);
        CHECK_RETURN(ret == RET_ERROR, "Client - Error closing pipe reading\n");

        ret = close(fd[WRITING]);
        CHECK_RETURN(ret == RET_ERROR, "Client - Error closing pipe writting\n");
        // Débloquage du master
        sell_mutex(sem_clients_id);
    }
    return EXIT_SUCCESS;
}
