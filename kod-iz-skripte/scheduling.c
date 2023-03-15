#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#define THREADS	5
static pthread_mutex_t monitor_handle = PTHREAD_MUTEX_INITIALIZER;

static void print_thread_scheduling_parameters ( long id )
{
	int policy;
	struct sched_param prio;

	pthread_mutex_lock ( &monitor_handle );
	pthread_getschedparam ( pthread_self(), &policy, &prio );
	printf ( "Thread %ld: policy=%d, prio=%d\n", id,
			 policy, prio.sched_priority );
	pthread_mutex_unlock ( &monitor_handle );
}

static void *thread_func ( void *param )
{
	print_thread_scheduling_parameters ( (long) param );
	return NULL;
}

int main ()
{
	long min, max, main_prio, i, policy;
	pthread_attr_t attr;
	pthread_t tid[THREADS];
	struct sched_param prio;

	print_thread_scheduling_parameters ( 0 );

	/*! Get priority range for "policy" */
	policy = SCHED_FIFO;
	min = sched_get_priority_min ( policy );
	max = sched_get_priority_max ( policy );

	/* set scheduling policy and priority for main thread */
	main_prio = prio.sched_priority = (min + max) / 2;
	if ( pthread_setschedparam ( pthread_self(), policy, &prio ) ) {
		perror ( "Error: pthread_setschedparam (root permission?)" );
		exit (1);
	}

	print_thread_scheduling_parameters ( 0 );

	/* define scheduling properties for new threads */
	pthread_attr_init ( &attr );
	pthread_attr_setinheritsched ( &attr, PTHREAD_EXPLICIT_SCHED );
	policy = SCHED_RR;
	pthread_attr_setschedpolicy ( &attr, policy );

	/* create threads */
	for ( i = 0; i < THREADS; i++ ) {
		prio.sched_priority = (min + i) % main_prio;
		pthread_attr_setschedparam ( &attr, &prio );
		if ( pthread_create(&tid[i], &attr, thread_func, (void *)(i+1)) ) {
			perror ( "Error: pthread_create" );
			exit (1);
		}
	}

	/* wait for threads to finish */
	for ( i = 0; i < THREADS; i++ )
		pthread_join ( tid[i], NULL );

	return 0;
}

/* Example run: (on single processor system !!!)
$ gcc scheduling.c -pthread -Wall
$ sudo ./a.out
Thread 0: policy=0, prio=0
Thread 0: policy=1, prio=50
Thread 5: policy=2, prio=5
Thread 4: policy=2, prio=4
Thread 3: policy=2, prio=3
Thread 2: policy=2, prio=2
Thread 1: policy=2, prio=1
*/
