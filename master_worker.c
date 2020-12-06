
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "myassert.h"

#include "master_worker.h"

// fonctions éventuelles proposées dans le .h
// intput -> pipe dans lequel le master lit la réponse du worker
// ouput -> pipe dans lequel le master écrit au worker
void create_pipes_master(int *input, int *output)
{
    int fdsInput[2], fdsOutput[2];
    int ret1 = pipe(fdsInput);
    int ret2 = pipe(fdsOutput);

    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR, "create_pipes_master - pipe not created\n");

    // TODO changer cette merde
    input[READING] = fdsInput[READING];
    input[WRITING] = fdsInput[WRITING];

    output[READING] = fdsOutput[READING];
    output[WRITING] = fdsOutput[WRITING];
}

void init_pipes_master(int input[], int output[])
{
    int ret = close(input[WRITING]);
    CHECK_RETURN(ret == RET_ERROR, "init_pipes_master - failed opening input pipe\n");

    ret = close(output[READING]);
    CHECK_RETURN(ret == RET_ERROR, "init_pipes_master - failed opening output pipe\n");
}

void create_worker(int workerIn, int workerOut)
{
    char n1[10], n2[10], prime[10];
    int ret1 = sprintf(n1, "%d", workerIn);
    int ret2 = sprintf(n2, "%d", workerOut);
    int ret3 = sprintf(prime, "%d", FIRST_PRIME_NUMBER);
    CHECK_RETURN(ret1 == RET_ERROR || ret2 == RET_ERROR || ret3 == RET_ERROR, "create_worker - failed convert int to char *\n");
    char *args[] = {"./worker", prime, n1, n2, NULL};

    int resFork = fork();
    CHECK_RETURN(resFork == RET_ERROR, "create_worker - failed fork master\n");

    if(resFork == 0)
    {
        int ret = execv("./worker", args);
        CHECK_RETURN(ret == RET_ERROR, "create_worker - failed exec worker\n");
        printf("Worker created\n");
    }
}

void close_pipes_master(int input[], int output[])
{
    int ret = close(input[READING]);
    CHECK_RETURN(ret == RET_ERROR, "close_pipes_master - failed closing input pipe\n");

    ret = close(output[WRITING]);
    CHECK_RETURN(ret == RET_ERROR, "close_pipes_master - failed closing output pipe\n");
}