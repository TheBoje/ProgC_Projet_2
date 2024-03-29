/************************************************************************
 * Projet Numéro 2
 * Programmation avancée en C
 *
 * Auteurs: Vincent Commin & Louis Leenart
 ************************************************************************/

#include "config.h"

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
    CHECK_RETURN(res == RET_ERROR, "master-client - failed to take mutex\n");
}

// vendre mutex
void sell_mutex(int sem_id)
{
    struct sembuf v = {0, 1, 0};
    int res = semop(sem_id, &v, 1);
    CHECK_RETURN(res == RET_ERROR, "master-client - failed to sell mutex\n");
}

// ouvrir les tubes nommés
void open_pipe(int side, int res[])
{
    myassert(side != SIDE_MASTER || side != SIDE_CLIENT, "master-client - wrong side input");
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
    CHECK_RETURN(res[READING] == RET_ERROR || res[WRITING] == RET_ERROR, "master-client - failed to open pipe\n");
}

// fermer les tubes nommés
void close_pipe(int *fd)
{
    int res = close(fd[READING]);
    CHECK_RETURN(res == RET_ERROR, "master-client - failed to close pipe\n");
    res = close(fd[WRITING]);
    CHECK_RETURN(res == RET_ERROR, "master-client - failed to close pipe\n");
}

// Destruction des tubes nommés
void destroy_pipe(char *pipe_name)
{
    int res = unlink(pipe_name);
    CHECK_RETURN(res == RET_ERROR, "master-client - failed to destroy pipe\n");
}