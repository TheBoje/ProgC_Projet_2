/************************************************************************
 * Projet Numéro 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "myassert.h"

#include "master_worker.h"

// intput -> pipe dans lequel le master lit la réponse du worker
// ouput -> pipe dans lequel le master écrit au worker
// Créer les pipes dans le master
void create_pipes_master(int *input, int *output)
{
    int fdsInput[2], fdsOutput[2];
    int ret1 = pipe(fdsInput);
    int ret2 = pipe(fdsOutput);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "create_pipes_master - pipe not created\n");

    input[READING] = fdsInput[READING];
    input[WRITING] = fdsInput[WRITING];

    output[READING] = fdsOutput[READING];
    output[WRITING] = fdsOutput[WRITING];
}

// Initialise les pipes entre le master et le worker
void init_pipes_master(int input[], int output[])
{
    int ret = close(input[WRITING]);
    CHECK_RETURN(ret == RET_ERROR, "init_pipes_master - failed to open input pipe\n");

    ret = close(output[READING]);
    CHECK_RETURN(ret == RET_ERROR, "init_pipes_master - failed to open output pipe\n");
}

// Clone le master et substitue le worker au master cloné
void create_worker(int workerIn, int workerOut)
{
    char n1[10], n2[10], prime[10]; // On initialise des tableaux de caractères pour les arguments du worker
    int ret1 = sprintf(n1, "%d", workerIn);
    int ret2 = sprintf(n2, "%d", workerOut);
    int ret3 = sprintf(prime, "%d", FIRST_PRIME_NUMBER);
    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR || ret3 == RET_ERROR, "create_worker - failed to convert int to char *\n");
    char *args[] = {"./worker", prime, n1, n2, NULL};

    int resFork = fork();
    CHECK_RETURN(resFork == RET_ERROR, "create_worker - failed to fork master\n");

    if (resFork == 0)
    {
        int ret = execv("./worker", args);
        CHECK_RETURN(ret == RET_ERROR, "create_worker - failed to exec worker\n");
    }
}

// Ferme les pipes entre le master et les workers
void close_pipes_master(int input[], int output[])
{
    int ret = close(input[READING]);
    CHECK_RETURN(ret == RET_ERROR, "close_pipes_master - failed to close input pipe\n");

    ret = close(output[WRITING]);
    CHECK_RETURN(ret == RET_ERROR, "close_pipes_master - failed to close output pipe\n");
}