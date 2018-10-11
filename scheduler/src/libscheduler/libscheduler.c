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
void * FCFS_cmp(void * job1, void * job2);
void * SJF_cmp(void * job1, void * job2);
void * PRI_cmp(void * job1, void * job2);

comparer determine_cmp(scheme_t scheme);

typedef struct core{
	priqueue_t q;
	int id;
	scheme_t scheme;
	bool idle;
} core;

typedef struct _job_t{
	int job_id;
	int arrival_time;
	int r_time;
	int priority;
	int start_time;
	int remaining_time;
} job_t;

core* cores;
int NUM_CORES = 0;
bool preemptive;
int current_time = 0;
int numJobs = 0;
int totalTurnTime = 0;
int totalWaitTime = 0;
int totalRespTime = 0;


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
	*cores = core[num_cores];

	for(int i = 0; i < num_cores; i++) {
		cores[i].id = i;
		cores[i].scheme = scheme;
		cores[i].idle = true;
		priqueue_init(&cores[i].q, determine_cmp(scheme));
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
			cmp = &FCFS_cmp;
			preemptive = false;
			break;
		case RR:
			cmp = &FCFS_cmp;
			preemptive = true;
			break;
		case SJF:
			cmp = &SJF_cmp;
			preemptive = false;
			break;
		case PSJF:
			cmp = &SJF_cmp;
			preemptive = true;
			break;
		case PRI:
			cmp = &PRI_cmp;
			preemptive = false;
			break;
		case PPRI:
			cmp = &PRI_cmp;
			preemptive = true;
			break;
		}
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
	int retv = 0;
	job_t * new_job;
	new_job->job_id = job_number;
	new_job->arrival_time = time_a;
	new_job->r_time = running_time;
	new_job->remaining_time = running_time;
	new_job->priority = priority;
	new_job->start_time = -1;

	totalWaitTime += time_a;

	//check cores and store
	for (int i = 0; i < NUM_CORES; i++) {
		if(cores[i].idle) {
	    priqueue_offer(cores[i].q, new_job);
			retv = -1;
			break;
		}
		retv = (priqueue_size(cores[retv].q) < priqueue_size(cores[i].q)) ? retv : i;
	}

	return retv;
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
	job_t * t_job = priqueue_poll(cores[core_id].q);
	numJobs += 1;
	totalWaitTime += time_e - (t_job->running_time - t_job->arrival_time);
	totalRespTime += t_job->start_time - t_job->arrival_time
	totalTurnTime += time_e - t_job->arrival_time;

	if (priqueue_peek(cores[core_id].q) == NULL) {
		return -1;
	}

	job_t * new_job = priqueue_peek(cores[core_id].q);
	// If the job doesn't have a start time, give it one
	if (new_job->start_time == -1){
		new_job->start_time = time_e;
	}
	return priqueue_peek(cores[core_id].q).job_id;
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
	job_t * job = priqueue_offer(cores[core_id].q, priqueue_poll(cores[core_id].q));
	job->remaining_time -= job->start_time;

	// Check to see if the next job exists
	if (priqueue_peek(cores[core_id].q) == NULL) {
		return -1;
	}

	job_t * new_job = priqueue_peek(cores[core_id].q);
	// If the job doesn't have a start time, give it one
	if (new_job->start_time == -1){
		new_job->start_time = time_e;
	}
	return (priqueue_peek(cores[core_id].q == NULL)) ? -1 : priqueue_peek(cores[core_id].q).job_id;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	float retv = totalRespTime / numJobs;
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
	float retv = (totalRespTime - totalWaitTime) / numJobs;
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
	float retv = totalRespTime / numJobs;
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
		priqueue_destroy(cores[i].q);
	}
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

}


void * FCFS_cmp(void * a, void * b){
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	return (job_a->arrival_time < job_b->arrival_time) ? a : b;
}

void * SJF_cmp(void * a, void * b){
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	return (job_a->r_time < job_b->r_time) ? a : b;
}

void * PRI_cmp(void * a, void * b){
	job_t * job_a = (job_t *)a;
	job_t * job_b = (job_t *)b;
	return (job_a->priority < job_b->priority) ? a : b;
}
