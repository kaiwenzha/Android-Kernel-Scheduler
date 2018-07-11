///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                      	 //
//  Description:                                                                        	 //
//      This file contains an test file for the basic part. In this file, we implement 		 //
//  the running of processtest.apk and you can change the scheduling policy for this task    //
//  and print out relative information.														 //
//                                                                                      	 //
///////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#define MAX_LEN 100
#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_WRR 6

void string_convert(char* sched, int number)
{
	switch (number)
	{
		case 0:
			strcpy(sched, "SCHED_NORMAL");
			break;
		case 1:
			strcpy(sched, "SCHED_FIFO");
			break;
		case 2:
			strcpy(sched, "SCHED_RR");
			break;
		case 6:
			strcpy(sched, "SCHED_WRR");
			break;
	}
}

void main(){
	// find the process, print info (PID, group and name)
	FILE *fp = popen("ps -P|grep processtest", "r");
	char buffer[MAX_LEN] = {'\0'};
	while(NULL != fgets(buffer, MAX_LEN, fp));
	pclose(fp);
	
	printf("==================INFO@processtest.apk==================\n");

	char *p;
	char tmp[100];

	strtok(buffer, " ");
	p = strtok(NULL, " ");
	int pid = atoi(p);
	printf("PID: %d\n", pid);

	strtok(NULL, " "); strtok(NULL, " "); strtok(NULL, " ");
	p = strtok(NULL, " ");
	if (strcmp(p, "fg") == 0) strcpy(tmp, "Foreground");
	else if (strcmp(p, "bg") == 0) strcpy(tmp, "Background");
	printf("Group: %s\n", tmp);

	strtok(NULL, " "); strtok(NULL, " "); strtok(NULL, " ");
	p = strtok(NULL, " ");
	printf("Process Name: %s\n", p);
	
	// change the scheduler
	int intended_sched, prio;
	printf("Scheduling algorithm list:\nSCHED_NORMAL\t0\nSCHED_FIFO\t1\nSCHED_RR\t2\nSCHED_WRR\t6\n\n");

	printf("Please enter the intended scheduler: ");
	scanf("%d", &intended_sched);

	printf("Please set the process's priority (0~99): ");
	scanf("%d", &prio);
	
	int prev_sched = sched_getscheduler(pid);
	struct sched_param param;
	//param.sched_priority = sched_get_priority_max(wanted_sched);
	param.sched_priority = prio;
	if(sched_setscheduler(pid, intended_sched, &param) == -1){
		fprintf(stderr, "SYSCALL: sched_setscheduler Failed!\n");
		exit(1);
	}

	char sched[20];
	string_convert(sched, prev_sched);
	printf("\nPrevious scheduler: %s\n", sched);

	int curr_sched = sched_getscheduler(pid);
	string_convert(sched, curr_sched);
	printf("Current scheduler: %s\n", sched);
	
	// print timeslice and priority
	struct timespec timeslice;
	if(sched_rr_get_interval(pid, &timeslice) == -1){
		fprintf(stderr, "SYSCALL: sched_rr_get_interval Failed!\n");
		exit(1);
	}

	sched_getparam(pid, &param);
	printf("Current scheduler's priority: %d\nCurrent scheduler's timeslice: %.2lf ms\n", param.sched_priority, timeslice.tv_sec * 1000.0 + timeslice.tv_nsec/1000000.0);
	
	printf("========================================================\n");
}