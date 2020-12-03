#ifndef CONFIG_H
#define CONFIG_H

// uncomment to use verbose mode
#define VERBOSE

#ifdef VERBOSE
    #define TRACE(x) fprintf(stderr, (x));
    #define TRACE2(x,p1) fprintf(stderr, (x), (p1));
#else
    #define TRACE(x)
    #define TRACE2(x,p1)
#endif

#define WRITING 1
#define READING 0

// Macro permettant de tester le retour de fonctions
#define CHECK_RETURN(c, m)  \
    if (c)                  \
    {                       \
        TRACE(m);           \
        exit(EXIT_FAILURE); \
    }

#define RET_ERROR -1

#endif
