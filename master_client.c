#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "myassert.h"

#include "master_client.h"

// prendre mutex
void take_mutex(int sem_id)
{
    struct sembuf p = {0, -1, 0};
    int res = semop(sem_id, &p, 1);
    if (res == -1)
    {
        fprintf(stderr, "Error take mutex");
    }
}

// vendre mutex
void sell_mutex(int sem_id)
{
    struct sembuf v = {0, 1, 0};
    int res = semop(sem_id, &v, 1);
    if (res == -1)
    {
        fprintf(stderr, "Error sell mutex");
    }
}

// ouvrir les tubes nommés
void open_pipe(int *fd, int side)
{
    assert(side != SIDE_MASTER || side != SIDE_CLIENT);
    int results[2];
    if (side == SIDE_MASTER)
    {
        results[0] = open(fd[0], O_WRONLY);
        results[1] = open(fd[1], O_RDONLY);
    }
    else if (side == SIDE_CLIENT)
    {
        results[0] = open(fd[0], O_WRONLY);
        results[1] = open(fd[1], O_RDONLY);
    }

    if (results[0] == -1 || results[1] == -1)
    {
        fprintf(stderr, "Error open pipes");
    }
}

// fermer les tubes nommés
void close_pipe(int *fd)
{
    int res = close(fd);
    if (res == -1)
    {
        fprintf(stderr, "Error close pipes");
    }
}

// Destruction des tubes nommés
void destroy_pipe(char *pipe_name)
{
    int res = unlink(pipe_name);
    if (res == -1)
    {
        fprintf(stderr, "Error destroy pipe");
    }
}