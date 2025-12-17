/*! Example that ilustrate priority inheritance
 *
 * Modeled by example from textbook L. Jelenkovic, Real-time systems, fig. 4.12.
 * (textbook is in croatian with title: Sustavi za rad u stvarnom vremenu)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

static void *task ( void *param );
static void simulate_processing ( int secs, char name, int part );
static void alarm_one_second ( union sigval a );
void count_iterations_within_second ();

#define THREADS     5
#define TASK_PARTS  4

/* task specification */
struct task_spec
{
	char name;        /* task name */
	int  prio;        /* task priority */
	int  start_delay; /* initial task delay */
	int  duration[TASK_PARTS]; /* part's duration in seconds */
	pthread_mutex_t *mon_a[TASK_PARTS];/* mutex to acquire at part start */
	pthread_mutex_t *mon_r[TASK_PARTS];/* mutex to release at part completion */
};

static pthread_mutex_t mon1, mon2;

static struct task_spec params[THREADS] = {
	{	'E', 1, 0,
		{ 3, 9, 0, 3 },
		{ NULL, &mon1, NULL, NULL},
		{ NULL, &mon1, NULL, NULL}
	},
	{	'D', 2, 6,
		{ 3, 0, 0, 3 },
		{ NULL, NULL, NULL, NULL },
		{ NULL, NULL, NULL, NULL }
	},
	{	'C', 3, 9,
		{ 3, 3, 3, 3 },
		{ NULL, &mon2, &mon1, NULL },
		{ NULL, NULL, &mon2, &mon1 }
	},
	{	'B', 4, 18,
		{ 3, 0, 0, 3 },
		{ NULL, NULL, NULL, NULL },
		{ NULL, NULL, NULL, NULL }
	},
	{	'A', 5, 21,
		{ 3, 3, 0, 3 },
		{ NULL, &mon2, NULL, NULL },
		{ NULL, &mon2, NULL, NULL }
	}
};

int main ()
{
	int min, max, main_prio, i;
	cpu_set_t cpu_mask;
	struct sched_param prio;
	pthread_attr_t thread_attr;
	pthread_mutexattr_t mutex_attr;
	pthread_t thr[THREADS];

	printf ( "Real time threads! Use with caution!\n"
			"Task messages migh not be printed on console at the time"
			"printf is called (because of real-time priorities)!\n" );

	/* count iterations for 1 s period, using timer */
	count_iterations_within_second ();

	/*
	 * to achieve preemption and priority inheritance limit all threads to
	 * single processor/core
	 */
	CPU_ZERO ( &cpu_mask );
	CPU_SET ( 0, &cpu_mask );
	sched_setaffinity ( (pid_t) 0, sizeof (cpu_set_t), &cpu_mask );
	/* threads created from this one will inherit above property */

	/* set SCHED_RR for main thread */
	min = sched_get_priority_min ( SCHED_RR );
	max = sched_get_priority_max ( SCHED_RR );
	main_prio = (min + max) / 2;
	prio.sched_priority = main_prio;
	if ( sched_setscheduler ( (pid_t) 0, SCHED_RR, &prio ) ) {
		perror ( "sched_setscheduler (started as admin?)" );
	}

	/* prepare properties for mutexes and init them */
	pthread_mutexattr_init ( &mutex_attr );
	pthread_mutexattr_settype ( &mutex_attr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutexattr_setprotocol ( &mutex_attr, PTHREAD_PRIO_INHERIT );
	pthread_mutex_init ( &mon1, &mutex_attr );
	pthread_mutex_init ( &mon2, &mutex_attr );

	/* prepare properties for new tasks (threads) */
	pthread_attr_init ( &thread_attr );
	pthread_attr_setinheritsched ( &thread_attr, PTHREAD_EXPLICIT_SCHED );
	pthread_attr_setschedpolicy ( &thread_attr, SCHED_FIFO );

	/* create threads */
	for ( i = 0; i < THREADS; i++ )
	{
		prio.sched_priority = params[i].prio;
		pthread_attr_setschedparam ( &thread_attr, &prio );

		pthread_create ( &thr[i], &thread_attr, task, &params[i] );
	}

	/* wait for threads to complete */
	for ( i = 0; i < THREADS; i++ )
		pthread_join ( thr[i], NULL );

	return 0;
}

/* task body */
static void *task ( void *param )
{
	int i;
	struct task_spec *p = param;
	struct timespec start_delay;

	start_delay.tv_nsec = 0;
	start_delay.tv_sec = p->start_delay;

	nanosleep ( &start_delay, NULL );

	printf("Task %c started (prio=%d)\n", p->name, p->prio);

	for ( i = 0; i < TASK_PARTS; i++ ) {
		if ( p->mon_a[i] != NULL )
			pthread_mutex_lock ( p->mon_a[i] );

		simulate_processing ( p->duration[i], p->name, i );

		if ( p->mon_r[i] != NULL )
			pthread_mutex_unlock ( p->mon_r[i] );
	}

	return NULL;
}

/* MAXCOUNT = number of iterations that should not be reached within 1 s */
#define MAXCOUNT    10000000000000ULL

volatile static unsigned long long counter = MAXCOUNT; /* iterations for 1 s */
volatile static unsigned int finish = 0;

/* simulate processing - use processor for given time in seconds */
static void simulate_processing ( int secs, char name, int part )
{
	unsigned long long k = 0;
	int j;

	for ( j = 1; j <= secs && !finish; j++ ) {
		printf ("Thread %c :: part %d (%d/%d)\n", name, part, j, secs );

		/* 1 second run time */
		for ( k = 0; k < counter && !finish; k++ )
			asm volatile ("":::"memory");
	}

	if ( finish ) { /* used only when counting iterations within 1 second */
		counter = k;
		printf ( "1 s = %lld iters\n", counter );
		finish = 0;
	}
}

/* alarm called upon 1 sec expiration (used only once, in initialization) */
static void alarm_one_second ( union sigval a )
{
	finish = 1;
}

void count_iterations_within_second ()
{
	struct sigevent event;
	timer_t timerid;
	struct itimerspec one_second;

	event.sigev_notify = SIGEV_THREAD;
	event.sigev_signo = 0;
	event.sigev_value.sival_int = 0;
	event.sigev_notify_function = alarm_one_second;
	event.sigev_notify_attributes = NULL;
	/* create timer (just create, its not started yet) */
	timer_create ( CLOCK_REALTIME, &event, &timerid );

	one_second.it_value.tv_sec = 1;
	one_second.it_value.tv_nsec = 0;
	one_second.it_interval.tv_sec = 0;
	one_second.it_interval.tv_nsec = 0;
	/* start timer, counting 1 seconds */
	timer_settime ( timerid, 0, &one_second, NULL );

	/* iterate until timer expires = count interations for 1 second */
	simulate_processing ( 1, 'G', 0 );

	/* timer expired, remove it */
	timer_delete ( timerid );
}

/* Example run: (on single processor system !!!)
$ gcc priority_inheritance.c -lpthread -Wall -lrt
$ sudo ./a.out
Thread G :: part 0 (1/1)
1 s = 411590154 iters
Task E started (prio=1)
Thread E :: part 0 (1/3)
Thread E :: part 0 (2/3)
Thread E :: part 0 (3/3)
Thread E :: part 1 (1/9)
Thread E :: part 1 (2/9)
Thread E :: part 1 (3/9)
Task D started (prio=2)
Thread D :: part 0 (1/3)
Thread D :: part 0 (2/3)
Thread D :: part 0 (3/3)
Task C started (prio=3)
Thread C :: part 0 (1/3)
Thread C :: part 0 (2/3)
Thread C :: part 0 (3/3)
Thread C :: part 1 (1/3)
Thread C :: part 1 (2/3)
Thread C :: part 1 (3/3)
Thread E :: part 1 (4/9)
Thread E :: part 1 (5/9)
Thread E :: part 1 (6/9)
Task B started (prio=4)
Thread B :: part 0 (1/3)
Thread B :: part 0 (2/3)
Thread B :: part 0 (3/3)
Task A started (prio=5)
Thread A :: part 0 (1/3)
Thread A :: part 0 (2/3)
Thread A :: part 0 (3/3)
Thread E :: part 1 (7/9)
Thread E :: part 1 (8/9)
Thread E :: part 1 (9/9)
Thread C :: part 2 (1/3)
Thread C :: part 2 (2/3)
Thread C :: part 2 (3/3)
Thread A :: part 1 (1/3)
Thread A :: part 1 (2/3)
Thread A :: part 1 (3/3)
Thread A :: part 3 (1/3)
Thread A :: part 3 (2/3)
Thread A :: part 3 (3/3)
Thread B :: part 3 (1/3)
Thread B :: part 3 (2/3)
Thread B :: part 3 (3/3)
Thread C :: part 3 (1/3)
Thread C :: part 3 (2/3)
Thread C :: part 3 (3/3)
Thread D :: part 3 (1/3)
Thread D :: part 3 (2/3)
Thread D :: part 3 (3/3)
Thread E :: part 3 (1/3)
Thread E :: part 3 (2/3)
Thread E :: part 3 (3/3)
$
*/
