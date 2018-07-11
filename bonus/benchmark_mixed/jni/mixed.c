//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      //
//  Description:                                                                        //
//      This file contains a mixed benchmark. It forks a specified number of            //
//  children, where each child generates an approximation of Pi utilizing               //
//  Monte-Carlo method as well as copies the source file once.                          //      
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
#include <sys/types.h>
#include <fcntl.h>

#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

#define DEFAULT_ITERATIONS 100000
#define DEFAULT_CHILDREN 20
#define RADIUS (RAND_MAX / 2)
#define MAX_CHILDREN 5000
#define MAX_FILENAME_LEN 100
#define MIN_ITERATIONS 100
#define MIN_ARGS 8
#define USAGE "Usage: time -p ./test_mixed ITERATIONS SCHED_POLICY PROC_COUNT BLOCK_SIZE TRAN_SIZE SRC DEST"

inline double dist(double x0, double y0, double x1, double y1)
{
    return sqrt(pow((x1 - x0), 2) + pow((y1 - y0), 2));
}

inline double zeroDist(double x, double y)
{
    return dist(0, 0, x, y);
}

double CPU(long);
void childTask(long, char*, char*, size_t, size_t);
void IO(size_t, ssize_t, char*, char*);
void parseCommandLine(int, char**, long*, int*, int*, size_t*, ssize_t*);

int main(int argc, char* argv[]){

    int i;
    int pid;
    long iterations;
    struct sched_param param;
    int policy;
    int child_count;
    int file_counter = 0;
    size_t block_size = 0;
    ssize_t tran_size = 0;
    char dest[MAX_FILENAME_LEN]= "";
    
    srand(time(0));

    parseCommandLine(argc, argv, &iterations, &policy, &child_count, &block_size, &tran_size);
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
            file_counter += 1;
            children[i] = pid;
        }
        else if (pid == 0) 
        {
            childTask(iterations, argv[6], argv[7], block_size, tran_size);
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

void parseCommandLine(int argc, char* argv[], long* iterations, int* policy, int* child_count, 
                        size_t* block_size, ssize_t* tran_size)
{
    if (argc < MIN_ARGS)
    {
        fprintf(stderr,"Too few arguments.\n%s", USAGE);
        exit(EXIT_FAILURE);
    }

    /* Set iterations if supplied */
    *iterations = atol(argv[1]);
    if (*iterations < MIN_ITERATIONS)
    {
        fprintf(stderr, "Bad iterations value. Must be at least 100.\n");
        exit(EXIT_FAILURE);
    }
    /* Set policy if supplied */
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
        fprintf(stderr, "Unhandeled scheduling policy\n");
        exit(EXIT_FAILURE);
    }

    *child_count = atoi(argv[3]);
    if ( *child_count < 1 || *child_count > MAX_CHILDREN )
    {
        fprintf(stderr, "Invalid process count: %d.\n", *child_count);
        exit(EXIT_FAILURE);
    }

    if (atoi(argv[4]) < 0)
    {
        fprintf(stderr, "Invalid block size.\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        *block_size = atoi(argv[4]);
    }

    if (atoi(argv[5]) < 0 || atoi(argv[5])/MIN_ITERATIONS < (int)(*block_size))
    {
        fprintf(stderr, "Invalid transfer size.\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        *tran_size = atoi(argv[5]);
    }
}

void childTask(long iterations, char* src, char* dest, size_t b_size, size_t t_size){
    int i;

    for (i = 0; i < MIN_ITERATIONS; i++)
    { 
        CPU(iterations / MIN_ITERATIONS);
        IO(b_size, t_size / MIN_ITERATIONS, src, dest);
    }
}

double CPU(long iter) 
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

    pCircle = inCircle/inSquare;
    piCalc = pCircle * 4.0;

    return piCalc;
}

void IO(size_t b_size, ssize_t t_size, char* src_name, char* dest_name) 
{
    int srcFD;
    int destFD;
    ssize_t bytes_read = 0;
    ssize_t bytes_written = 0;
    ssize_t total_bytes_written = 0;
    char* buf = NULL;
    /* Open source file. */
    srcFD = open(src_name, O_RDONLY);
    if ( srcFD < 0) 
    {
        fprintf(stderr, "There was an error opening the source file.\n");
        perror("");
        exit(EXIT_FAILURE);
    }
    /* Open/create dest file. */
    destFD = open(dest_name, O_WRONLY | O_CREAT);
    if ( destFD < 0) 
    {
        fprintf(stderr, "There was an error opening the destination file.\n");
        perror("");
        exit(EXIT_FAILURE);
    }

    /* malloc buffer */
    buf = malloc(b_size * sizeof(char));
    if (buf == NULL) {
        fprintf(stderr, "Error allocating buffer.\n");
        exit(EXIT_FAILURE);
    }
    do 
    {
        bytes_read = read(srcFD, buf, b_size);
        if (bytes_read < 0)
        {
            perror("Error reading source file.");
            exit(EXIT_FAILURE);
        }

        if (bytes_read > 0) 
        {
            bytes_written = write(destFD, buf, bytes_read);
            if (bytes_written < 0){
                perror("Error writing to output file.");
                exit(EXIT_FAILURE);
            } 
            else 
            {
                total_bytes_written += bytes_written;
            }
        }
        if (bytes_read != (ssize_t)b_size)
        {
            if (lseek(srcFD, 0, SEEK_SET))
            {
                perror("Error seeking to beginning of file.");
                exit(EXIT_FAILURE);
            }
        }
    } while(total_bytes_written < (ssize_t)t_size);

    if(close(srcFD))
    {
        perror("Error closing source file.");
        exit(EXIT_FAILURE);
    }
    if(close(destFD))
    {
        perror("Error closing dest file.");
        exit(EXIT_FAILURE);
    }

    free(buf);
}
