/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
int FCFS_cmp(void * job1, void * job2);
int SJF_cmp(void * job1, void * job2);
int PSJF_cmp(void * job1, void * job2);
int PRI_cmp(void * job1, void * job2);
int RR_cmp(void * job1, void * job2);

comparer determine_cmp(scheme_t scheme);

typedef struct job_t{
	int job_id;
	int priority;
	float arrival_time;
	float running_time;
	float start_time;
	float remaining_time;
	float last_start_time;
} job_t;

typedef struct core{
	int id;
	bool idle;
	job_t * running_job;
} core;

core* cores;
priqueue_t wait_queue;
int NUM_CORES = 0;
bool preemptive;
int current_time = 0;
int numJobs = 0;
float totalTurnTime = 0;
float totalWaitTime = 0;
float totalRespTime = 0;


/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int num_cores, scheme_t scheme)
{
	cores = malloc(sizeof(core) * num_cores);
	priqueue_init(&wait_queue, determine_cmp(scheme));
	NUM_CORES = num_cores;

	for(int i = 0; i < num_cores; i++) {
		cores[i].id = i;
		cores[i].idle = true;
		cores[i].running_job = NULL;
	}
}

/**
	Helper function to determine compare function to use
	@param scheme_t
	@return comparer
*/
comparer determine_cmp(scheme_t scheme)
{
	comparer cmp;
	switch (scheme){
		case FCFS:
			cmp = (comparer)&FCFS_cmp;
			preemptive = false;
			break;
		case RR:
			cmp = (comparer)&RR_cmp;
			preemptive = true;
			break;
		case SJF:
			cmp = (comparer)&SJF_cmp;
			preemptive = false;
			break;
		case PSJF:
			cmp = (comparer)&PSJF_cmp;
			preemptive = true;
			break;
		case PRI:
			cmp = (comparer)&PRI_cmp;
			preemptive = false;
			break;
		case PPRI:
			cmp = (comparer)&PRI_cmp;
			preemptive = true;
			break;
		}
		return cmp;
}

/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time_a the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time_a, int running_time, int priority)
{
	job_t * new_job = malloc(sizeof(job_t));
	new_job->job_id = job_number;
	new_job->arrival_time = time_a;
	new_job->running_time = running_time;
	new_job->remaining_time = running_time;
	new_job->priority = priority;
	new_job->start_time = -1;
	new_job->last_start_time = -1;

	//check cores and store
	for (int i = 0; i < NUM_CORES; i++) {
		// If theres an idle queue add it there and return that core id
		if(cores[i].idle) {
			new_job->start_time = time_a;
			new_job->last_start_time = time_a;
			cores[i].running_job = new_job;
			cores[i].idle = false;
			return i;
		}
	}
	// if its a preemptive scheme and need to add it to a queue, add it to the shortest queue and return
	if (preemptive){
		//find least preferential job

		// update each cores running time
		for (int i = 0; i < NUM_CORES; i++){
			cores[i].running_job->remaining_time = cores[i].running_job->remaining_time - (time_a - cores[i].running_job->last_start_time);
			cores[i].running_job->last_start_time = time_a;
		}
		priqueue_t t_q;
		int core_to_assign = -1;
		priqueue_init(&t_q, wait_queue.cmp);
		for (int i = 0; i < NUM_CORES; i ++){
			priqueue_offer(&t_q, cores[i].running_job);
		}
		job_t * temp_job = (job_t *)priqueue_remove_at(&t_q, NUM_CORES-1);
		printf("Job from back is : %d\n", temp_job->job_id);
		priqueue_destroy(&t_q);
		// end getting job
		//
		// printf("Temp job remaining time before change: %f\n", temp_job->remaining_time);
		// printf("Time a : %d   temp job start time: %f\n", time_a, temp_job->start_time);

		// printf("Temp job remaining time after change: %f\n", temp_job->remaining_time);
		for (int i  = 0; i < NUM_CORES; i++){
			if (temp_job == cores[i].running_job){
				core_to_assign = i;
				break;
			}
		}

		//compare jobs here
		//printf("REMAING TIMES: %f,%f\n", new_job->remaining_time, temp_job->remaining_time);
		//temp_job->remaining_time = temp_job->remaining_time - temp_job->running_time;
		//printf("temp job last start time: %f\n", temp_job->last_start_time);

		// printf("REMAING TIMES: %f,%f\n", new_job->remaining_time, temp_job->remaining_time);
		if (wait_queue.cmp(new_job, temp_job) == -1) {
			temp_job->last_start_time = -1;
			priqueue_offer(&wait_queue, temp_job);
			if(new_job->start_time == -1) {
				new_job->start_time = time_a;
				new_job->last_start_time = time_a;
			}
			cores[core_to_assign].running_job = new_job;
			// printf("REMAING TIMES: %f,%f\n", new_job->remaining_time, temp_job->remaining_time);
			return core_to_assign;
		}
	}
	// if neither of the other two conditions met, add it to a global queue and return -1

	priqueue_offer(&wait_queue, new_job);
	return -1;
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time_e)
{
	//job_t * t_job = (job_t *)priqueue_poll(&cores[core_id].q);
	job_t * t_job = cores[core_id].running_job;
	numJobs += 1;
	totalWaitTime += ((time_e - t_job->running_time) - t_job->arrival_time);
	totalRespTime += t_job->start_time - t_job->arrival_time;
	totalTurnTime += time_e - t_job->arrival_time;

	// Free the job
	free(t_job);

	if (priqueue_peek(&wait_queue) == NULL) {
		cores[core_id].idle = true;
		return -1;
	}

	job_t * new_job = (job_t *)priqueue_poll(&wait_queue);
	// If the job doesn't have a start time, give it one
	if (new_job->start_time == -1) { //(new_job->start_time == -1){
		new_job->start_time = time_e;
	}
	// job is starting so set its last start time to now
	new_job->last_start_time = time_e;
	// set the new running job
	cores[core_id].running_job = new_job;
	return new_job->job_id;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time_c)
{
	// get the preempted job
	job_t * job = cores[core_id].running_job;
	// set the jobs remaining time
	job->remaining_time -= (time_c - job->last_start_time);
	// get job id
	priqueue_offer(&wait_queue, job);
	// Check to see if the next job exists
	if (priqueue_peek(&wait_queue) == NULL) {
		cores[core_id].idle = true;
		return -1;
	}

	job_t * new_job = (job_t *)priqueue_poll(&wait_queue);

	// If the job doesn't have a start time, give it one
	if (new_job->start_time == -1){
		new_job->start_time = time_c;
		new_job->last_start_time = time_c;
	}
	cores[core_id].running_job = new_job;
	return new_job->job_id;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	float retv = (float)totalWaitTime / (float)numJobs;
	return retv;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	float retv = (float)totalTurnTime / (float)numJobs;
	return retv;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	float retv = (float)totalRespTime / (float)numJobs;
	return retv;
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	for (int i = 0; i < NUM_CORES; i ++){
		cores[i].running_job = NULL;
	}
	priqueue_destroy(&wait_queue);
	free(cores);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled.
	Furthermore, we have also listed the current state of the job (either running on a given core or idle).
	For example, if we have a non-preemptive algorithm and job(id=4) has began running,
	job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
	printf("RUNNING JOBS:\n");
	job_t * print_job;
	for (int i = 0; i < NUM_CORES; i++){
		print_job = cores[i].running_job;
		if (cores[i].idle){
			printf("CORE IS IDLE");
		}
		else{
			printf("Core #%d: job_id: %d  job_priority: %d   remaining_time: %f\n", i, print_job->job_id, print_job->priority, print_job->remaining_time);
		}
	}

	printf("\nJOBS IN QUEUE:\n");
	if (priqueue_size(&wait_queue) <= 0){
		printf("NO JOBS IN QUEUE\n");
	}
	for (int i = 0; i < priqueue_size(&wait_queue); i++){
		print_job = (job_t *)(priqueue_at(&wait_queue, i));
		printf("job_id: %d   job_priority: %d    remaining_time: %f\n", print_job->job_id, print_job->priority, print_job->remaining_time);
	}
}


int FCFS_cmp(void * a, void * b){
	int retv;
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;

	if (job_a->arrival_time < job_b->arrival_time){
		retv = -1;
	}else if(job_a->arrival_time == job_b->arrival_time){
		retv = 0;
	}else{
		retv = 1;
	}
	return retv;
}

int SJF_cmp(void * a, void * b){
	int retv;
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	if (job_a->running_time < job_b->running_time){
		retv = -1;
	}else if(job_a->running_time == job_b->running_time){
		retv = 0;
	}else{
		retv = 1;
	}
	return retv;
}

int PSJF_cmp(void * a, void * b){
	int retv;
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	if (job_a->remaining_time < job_b->remaining_time){
		retv = -1;
	}else if(job_a->remaining_time == job_b->remaining_time){
		retv = 0;
	}else{
		retv = 1;
	}
	return retv;
}

int PRI_cmp(void * a, void * b){
	int retv;
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	if (job_a->priority < job_b->priority){
		retv = -1;
	}else if(job_a->priority == job_b->priority){
		retv =  0;
	}else{
		retv =  1;
	}
	return retv;
}

int RR_cmp(void * a, void * b){
	return 1;
}

void increment_timer(int time_c) {
	time_c++;
}
