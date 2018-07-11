//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      //
//  Description:                                                                        //
//      This file contains an io bound benchmark. It forks a specified number of        //
//  children, where each child copies the source file once.                             //                                           //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

#define MAX_FILENAME_LEN 100
#define MIN_ARGS 7
#define MAX_CHILDREN 5000
#define USAGE "Usage: time -p ./test_iobound SCHED_POLICY SRC DEST BLOCK_SIZE TRAN_SIZE PROC_COUNT\n"

void childTask(size_t, size_t, char*, char*);
void parseCommandLine(int, char**, int*, int*, size_t*, ssize_t*, FILE*);

int main(int argc, char* argv[])
{

    int i;
    int pid;
    struct sched_param param;
    int policy;
    int child_count;
    size_t block_size;
    ssize_t tran_size;
    int file_counter = 0;
    char dest_name[MAX_FILENAME_LEN];
    FILE* src;

    /* Parge command line */
    parseCommandLine(argc, argv, &policy, &child_count, &block_size, &tran_size, src);

    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);

    /* Set new scheduler policy */
    printf("Current Scheduling Policy: %d\n", sched_getscheduler(0));
    printf("Setting Scheduling Policy to: %d\n", policy);
    if (sched_setscheduler(0, policy, &param))
    {
        perror("Error setting scheduler policy");
        exit(EXIT_FAILURE);
    }
    printf("New Scheduling Policy: %d\n", sched_getscheduler(0));

    int* children = malloc(sizeof(int) * child_count);
    printf("Starting forking children...\n");
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
            childTask(block_size, tran_size, argv[2], argv[3]);
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
void parseCommandLine(int argc, char* argv[], int* policy, int* child_count, 
                        size_t* block_size, ssize_t* tran_size, FILE* src)
{
    if (argc < MIN_ARGS)
    {
        fprintf(stderr, "Not enough arguments: %d\n", argc - 1);
        fprintf(stderr, USAGE);
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "SCHED_WRR"))
    {
        *policy = SCHED_WRR;
    } 
    else if (!strcmp(argv[1], "SCHED_FIFO"))
    {
        *policy = SCHED_FIFO;
    } 
    else if (!strcmp(argv[1], "SCHED_RR"))
    {
        *policy = SCHED_RR;
    } 
    else
    {
        fprintf(stderr, "Unhandled scheduling policy\n");
        exit(EXIT_FAILURE);
    }

    /* Test that you can open the src file */
    src = fopen(argv[2], "r");
    if (src == NULL) 
    {
        fprintf(stderr, "There was an error opening the source file: %s\n", argv[2]);
        perror("");
        exit(EXIT_FAILURE);
    }
    fclose(src);

    if (atoi(argv[4]) < 0)
    {
        fprintf(stderr, "Invalid block size.\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        *block_size = atoi(argv[4]);
    }

    if (atoi(argv[5]) < 0 || atoi(argv[5]) < (int)(*block_size))
    {
        fprintf(stderr, "Invalid transfer size.\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        *tran_size = atoi(argv[5]);
    }

    *child_count = atoi(argv[6]);
    if ( *child_count < 1 || *child_count > MAX_CHILDREN )
    {
        fprintf(stderr, "Unreasonable children count\n");
        exit(EXIT_FAILURE);
    }
}

void childTask(size_t b_size, size_t t_size, char* src_name, char* dest_name) 
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
    destFD = open(dest_name, O_WRONLY | O_CREAT );
    if ( destFD < 0) 
    {
        fprintf(stderr, "There was an error opening the destination file.\n");
        perror("");
        exit(EXIT_FAILURE);
    }

    /* malloc buffer */
    buf = malloc(b_size * sizeof(char));
    if (buf == NULL) 
    {
        fprintf(stderr, "Error allocating buffer.\n");
        exit(EXIT_FAILURE);
    }
    do {
        bytes_read = read(srcFD, buf, b_size);
        if (bytes_read < 0)
        {
            perror("Error reading source file.");
            exit(EXIT_FAILURE);
        }

        if (bytes_read > 0) 
        {
            bytes_written = write(destFD, buf, bytes_read);
            if (bytes_written < 0)
            {
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
    } while (total_bytes_written < (ssize_t)t_size);

    if (close(srcFD))
    {
        perror("Error closing source file.");
        exit(EXIT_FAILURE);
    }
    if (close(destFD))
    {
        perror("Error closing dest file.");
        exit(EXIT_FAILURE);
    }

    free(buf);
}
