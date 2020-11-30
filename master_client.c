#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>

#include "myassert.h"

#include "master_client.h"

// prendre mutex
void take_mutex(int sem_id)
{
    struct sembuf p = {0, -1, 0};
    int res = semop(sem_id, &p, 1);
    if (res == RET_ERROR)
    {
        fprintf(stderr, "Error take mutex\n");
        exit(EXIT_FAILURE);
    }
}

// vendre mutex
void sell_mutex(int sem_id)
{
    struct sembuf v = {0, 1, 0};
    int res = semop(sem_id, &v, 1);
    if (res == RET_ERROR)
    {
        fprintf(stderr, "Error sell mutex\n");
        exit(EXIT_FAILURE);
    }
}

// ouvrir les tubes nommés
void open_pipe(int side, int res[])
{
    myassert(side != SIDE_MASTER || side != SIDE_CLIENT, "Wrong side input");
    if (side == SIDE_MASTER)
    {
        res[READING] = open(PIPE_MASTER_INPUT, O_RDONLY, "0660");
        res[WRITING] = open(PIPE_MASTER_OUTPUT, O_WRONLY, "0660");
    }
    else if (side == SIDE_CLIENT)
    {
        res[WRITING] = open(PIPE_CLIENT_OUTPUT, O_WRONLY, "0660");
        res[READING] = open(PIPE_CLIENT_INPUT, O_RDONLY, "0660");
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    if (res[READING] == RET_ERROR || res[WRITING] == RET_ERROR)
    {
        fprintf(stderr, "Error open pipes\n");
        exit(EXIT_FAILURE);
    }
}

// fermer les tubes nommés
void close_pipe(int *fd)
{
    int res = close(fd[0]);
    if (res == RET_ERROR)
    {
        fprintf(stderr, "Error close pipes\n");
        exit(EXIT_FAILURE);
    }
    res = close(fd[1]);
    if (res == RET_ERROR)
    {
        fprintf(stderr, "Error close pipes\n");
        exit(EXIT_FAILURE);
    }
}

// Destruction des tubes nommés
void destroy_pipe(char *pipe_name)
{
    int res = unlink(pipe_name);
    if (res == RET_ERROR)
    {
        fprintf(stderr, "Error destroy pipe\n");
        exit(EXIT_FAILURE);
    }
}