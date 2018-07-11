/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//  Description:                                                                               //
//      This file contains an file that behaves as a driver program that executes three kinds  //
//  of benchmark under a sequence of process numbers and print out corresponding information   //
//  for later visualization part.                                                              //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPU_CONFIG
#define IO_CONFIG
#define MIXED_CONFIG

#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

#define cpu_min 20
#define cpu_max 100
#define cpu_stride 5

#define io_min 20
#define io_max 100
#define io_stride 5

#define mixed_min 20
#define mixed_max 100
#define mixed_stride 5

FILE *fp;
char inst[1000];
char ret[1000];
double realtime, usrtime, systime;

void convert_to_string(int policy)
{
    switch (policy)
    {
        case 1:
            strcpy(ret, "SCHED_FIFO");
            break;
        case 2:
            strcpy(ret, "SCHED_RR");
            break;
        case 6:
            strcpy(ret, "SCHED_WRR");
            break;
    }
}

void print_cpubound(int policy, FILE *fp)
{
    int i;

    convert_to_string(policy);
    for (i = cpu_min; i <= cpu_max; i += cpu_stride)
    {
    	memset(inst, sizeof(inst), 0);
    	memset(ret, sizeof(ret), 0);

    	printf("PROC_COUNT = %d\n", i);
        sprintf(inst, "time -p ./test_cpubound 100000 %s %d", ret, i);
        fp = popen(inst, "r");
        pclose(fp);
    }
}

void print_iobound(int policy, FILE *fp)
{
    int i;

    convert_to_string(policy);
    for (i = io_min; i <= io_max; i += io_stride)
    {
    	memset(inst, sizeof(inst), 0);
    	memset(ret, sizeof(ret), 0);

    	printf("PROC_COUNT = %d\n", i);
        sprintf(inst, "time -p ./test_iobound %s /data/misc/src/data_in /data/misc/src/data_out 2000 5000000 %d", ret, i);
        fp = popen(inst, "r");
        pclose(fp);
    }
}

void print_mixed(int policy, FILE *fp)
{
    int i;

    convert_to_string(policy);
    for (i = mixed_min; i <= mixed_max; i += mixed_stride)
    {
    	memset(inst, sizeof(inst), 0);
    	memset(ret, sizeof(ret), 0);

    	printf("PROC_COUNT = %d\n", i);
        sprintf(inst, "time -p ./test_mixed 100000 %s %d 2000 5000000 /data/misc/src/data_in /data/misc/src/_data_out", ret, i);
        fp = popen(inst, "r");
        pclose(fp);
    }
}

void cpu_output()
{
	printf("Here is CPU Results!\n");
	printf("SCHED_FIFO:\n");
    print_cpubound(SCHED_FIFO, fp);
    printf("SCHED_RR:\n");
    print_cpubound(SCHED_RR, fp);
    printf("SCHED_WRR:\n");
    print_cpubound(SCHED_WRR, fp);
}

void io_output()
{
	printf("Here is IO Results!\n");
	printf("SCHED_FIFO:\n");
    print_iobound(SCHED_FIFO, fp);
    printf("SCHED_RR:\n");
    print_iobound(SCHED_RR, fp);
    printf("SCHED_WRR:\n");
    print_iobound(SCHED_WRR, fp);
}

void mixed_output()
{
	printf("Here is MIXED Results!\n");
	printf("SCHED_FIFO:\n");
    print_mixed(SCHED_FIFO, fp);
    printf("SCHED_RR:\n");
    print_mixed(SCHED_RR, fp);
    printf("SCHED_WRR:\n");
    print_mixed(SCHED_WRR, fp);
}

int main()
{
	#ifdef CPU_CONFIG
		cpu_output();
	#endif

	#ifdef IO_CONFIG
		io_output();
	#endif

	#ifdef MIXED_CONFIG
		mixed_output();
	#endif
}
