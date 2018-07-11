//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      //
//  Description:                                                                        //
//      This file contains a CPU-bound benchmark. It forks a specified number of        //
//  children, where each child generates an approximation of Pi utilizing Monte-Carlo   //
//  method.                                                                             //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

#define DEFAULT_ITERATIONS 100000
#define DEFAULT_CHILDREN 20
#define MAX_CHILDREN 5000
#define RADIUS (RAND_MAX / 2)
#define USAGE "Usage: time -p ./test_cpubound ITERATIONS SCHED_POLICY PROC_COUNT\n"

/* distance between two points */
inline double dist(double x0, double y0, double x1, double y1)
{
    return sqrt(pow((x1 - x0), 2) + pow((y1 - y0), 2));
}

/* distance to zero-point */
inline double zeroDist(double x, double y)
{
    return dist(0, 0, x, y);
}

double calcPi(long);
void childTask(long);
void parseCommandLine(int, char**, long*, int*, int*);

int main(int argc, char* argv[])
{

    int i;
    int pid;
    long iterations;
    struct sched_param param;
    int policy;
    int child_count;

    srand(time(0)); /* initialize random seeds */

    /* Parge command line */
    parseCommandLine(argc, argv, &iterations, &policy, &child_count);
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);

    /* Set new scheduler policy */
    printf("Current Scheduling Policy: %d\n", sched_getscheduler(0));
    printf("Setting Scheduling Policy to: %d\n", policy);

    if (sched_setscheduler(0, policy, &param))
    {
        perror("Error setting scheduler policy\n");
        exit(EXIT_FAILURE);
    }
    printf("New Scheduling Policy: %d\n", sched_getscheduler(0));

    int* children = malloc(sizeof(int) * child_count);
    printf("Start forking children...\n");
    fflush(0); 

    for (i = 0; i < child_count; i++)
    {
        pid = fork();
        if (pid > 0)
        {
            children[i] = pid;
        } 
        else if (pid == 0) 
        {
            childTask(iterations);
            _exit(0);
        } 
        else if (pid < 0) 
        {
            fprintf(stderr, "Error forking.\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < child_count; i++)
    {
        waitpid(children[i], NULL, 0);
    }
    free(children);

    return 0;
}

/* Parse program arguments to select iterations and policy */
void parseCommandLine(int argc, char* argv[], long* iterations, int* policy, int* child_count)
{
    if (argc != 4) 
    {
        fprintf(stderr, USAGE);
        exit(EXIT_FAILURE);
    }
    /* Set default iterations if not supplied */
    if (argc < 2)
    {
        *iterations = DEFAULT_ITERATIONS;
    }
    /* Set default policy if not supplied */
    if (argc < 3)
    {
        *policy = SCHED_WRR;
    }
    /* Set iterations if supplied */
    if (argc > 1)
    {
        *iterations = atol(argv[1]);
        if (*iterations < 1)
        {
            fprintf(stderr, "Bad iterations value\n");
            fprintf(stderr, USAGE);
            exit(EXIT_FAILURE);
        }
    }
    /* Set policy if supplied */
    if (argc > 2)
    {
        if (!strcmp(argv[2], "SCHED_WRR"))
        {
            *policy = SCHED_WRR;
        }
        else if (!strcmp(argv[2], "SCHED_FIFO"))
        {
            *policy = SCHED_FIFO;
        }
        else if (!strcmp(argv[2], "SCHED_RR"))
        {
            *policy = SCHED_RR;
        }
        else
        {
            fprintf(stderr, "Unhandled scheduling policy\n");
            fprintf(stderr, USAGE);
            exit(EXIT_FAILURE);
        }
    }

    if (argc > 3) 
    {
        *child_count = atoi(argv[3]);
        if ( *child_count < 1 || *child_count > MAX_CHILDREN )
        {
            fprintf(stderr, "Unreasonable children count\n");
            fprintf(stderr, USAGE);
            exit(EXIT_FAILURE);
        }
    } 
    else 
    {
        *child_count = DEFAULT_CHILDREN;
    }

}

void childTask(long iterations) 
{
    double pi = calcPi(iterations);

    (void) pi;
}

double calcPi(long iter) 
{
    long i;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;

    /* Calculate pi using Monte-Carlo method across all iterations*/
    for (i = 0; i < iter; i++)
    {
        x = (random() % (RADIUS * 2)) - RADIUS;
        y = (random() % (RADIUS * 2)) - RADIUS;
        if (zeroDist(x,y) < RADIUS)
        {
            inCircle++;
        }
        inSquare++;
    }

    /* Finish calculation */
    pCircle = inCircle/inSquare;
    piCalc = pCircle * 4.0;

    return piCalc;
}
