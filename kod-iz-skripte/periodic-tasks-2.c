/*! Example: periodic tasks with real time scheduling */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define THREADS	5

static pthread_mutex_t monitor_handle = PTHREAD_MUTEX_INITIALIZER;

struct timespec t0, tx0;

#define TIMESPEC_ADD(A,B) /* A += B */ \
do {                                   \
    (A).tv_sec += (B).tv_sec;          \
    (A).tv_nsec += (B).tv_nsec;        \
    if ( (A).tv_nsec >= 1000000000 ) { \
        (A).tv_sec++;                  \
        (A).tv_nsec -= 1000000000;     \
    }                                  \
} while (0)

#define TIMESPEC_SUB(A,B) /* A -= B */ \
do {                                   \
    (A).tv_sec -= (B).tv_sec;          \
    (A).tv_nsec -= (B).tv_nsec;        \
    if ( (A).tv_nsec < 0 ) {           \
        (A).tv_sec--;                  \
        (A).tv_nsec += 1000000000;     \
    }                                  \
} while (0)

#define TIMESTAMP(ID,MSG,ITER)                         \
do {                                                   \
    pthread_mutex_lock ( &monitor_handle );            \
    clock_gettime ( CLOCK_REALTIME, &t );              \
    TIMESPEC_SUB ( t, t0 );                            \
    printf ( "[%02ld:%06ld] Thread %ld: %s iter=%d\n", \
        t.tv_sec % 100, t.tv_nsec/1000, ID,MSG,ITER ); \
    pthread_mutex_unlock ( &monitor_handle );          \
} while (0)

/* adjust counter per processor power! experiment with this value */
#define ITERCOUNT 30000000ULL

void *thread_func ( void *param )
{
	long thread_id = (long) param;
	int iter;
	struct timespec next_act, period, t;
	unsigned long long cnt;

	/* period = 500 ms * thread_id */
	period.tv_sec = thread_id / 2;
	period.tv_nsec = ( thread_id % 2 ) * 500000000;

	next_act = tx0;

	for ( iter = 0; iter < 100; iter++ )
	{
		TIMESTAMP ( thread_id, "START", iter );
		for ( cnt = 0; cnt < thread_id * ITERCOUNT; cnt++ )
			asm volatile ("":::"memory");
		TIMESTAMP ( thread_id, "END", iter );

		TIMESPEC_ADD ( next_act, period );
		clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &next_act, NULL );
	}

	return NULL;
}

int main ()
{
	long min, max, main_prio, i, policy;
	struct sched_param prio;
	pthread_attr_t attr;
	pthread_t tid[THREADS];

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

	/* define scheduling properties for new threads */
	pthread_attr_init ( &attr );
	pthread_attr_setinheritsched ( &attr, PTHREAD_EXPLICIT_SCHED );
	policy = SCHED_RR;
	pthread_attr_setschedpolicy ( &attr, policy );

	clock_gettime ( CLOCK_REALTIME, &t0 ); /* "zero" time */
	clock_gettime ( CLOCK_MONOTONIC, &tx0 );

	/* create threads */
	for ( i = 0; i < THREADS; i++ ) {
		prio.sched_priority = (min + THREADS - i) % main_prio;
		pthread_attr_setschedparam ( &attr, &prio );
		if ( pthread_create(&tid[i], &attr, thread_func, (void *)(i+1)) ){
			perror ( "Error: pthread_create" );
			return 1;
		}
	}

	/* wait for threads to finish */
	for ( i = 0; i < THREADS; i++ )
		pthread_join ( tid[i], NULL );

	return 0;
}

/* Example run: (on single processor system !!!)
$ gcc periodic-tasks-2.c -pthread -Wall
$ sudo ./a.out
[00:000156] Thread 1: START iter=0
[00:076067] Thread 1: END iter=0
[00:076105] Thread 2: START iter=0
[00:228506] Thread 2: END iter=0
[00:228540] Thread 3: START iter=0
[00:456775] Thread 3: END iter=0
[00:456806] Thread 4: START iter=0
[00:500974] Thread 1: START iter=1
[00:576565] Thread 1: END iter=1
[00:835491] Thread 4: END iter=0
[00:835522] Thread 5: START iter=0
[01:000119] Thread 1: START iter=2
[01:075922] Thread 1: END iter=2
[01:075952] Thread 2: START iter=1
[01:227702] Thread 2: END iter=1
[01:442982] Thread 5: END iter=0
[01:500057] Thread 1: START iter=3
[01:576410] Thread 1: END iter=3
...
*/
