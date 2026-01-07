/*! Example: periodic tasks with normal (not real-time) scheduling */

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define THREADS	3

static pthread_mutex_t monitor_handle = PTHREAD_MUTEX_INITIALIZER;
static struct timespec t0, tx0;

#define TIMESPEC_ADD(A,B) /* A += B */ \
do {                                   \
    (A).tv_sec += (B).tv_sec;          \
    (A).tv_nsec += (B).tv_nsec;        \
    if ((A).tv_nsec >= 1000000000) {   \
        (A).tv_sec++;                  \
        (A).tv_nsec -= 1000000000;     \
    }                                  \
} while (0)

#define TIMESPEC_SUB(A,B) /* A -= B */ \
do {                                   \
    (A).tv_sec -= (B).tv_sec;          \
    (A).tv_nsec -= (B).tv_nsec;        \
    if ((A).tv_nsec < 0) {             \
        (A).tv_sec--;                  \
        (A).tv_nsec += 1000000000;     \
    }                                  \
} while (0)

#define TIMESTAMP(ID,MSG,ITER)                         \
do {                                                   \
    pthread_mutex_lock(&monitor_handle);               \
    clock_gettime(CLOCK_REALTIME, &t);                 \
    TIMESPEC_SUB(t, t0);                               \
    printf("[%02ld:%06ld] Thread %ld: %s iter=%d\n",   \
        t.tv_sec % 100, t.tv_nsec/1000, ID,MSG,ITER);  \
    pthread_mutex_unlock(&monitor_handle);             \
} while (0)

/* adjust counter per processor power! experiment with this value */
#define ITERCOUNT 30000000ULL

static void *thread_func(void *param)
{
	long thread_id = (long) param;
	int iter;
	struct timespec next_act, period, t;
	unsigned long long cnt;

	/* period = 500 ms * thread_id */
	period.tv_sec = thread_id / 2;
	period.tv_nsec = (thread_id % 2) * 500000000;

	next_act = tx0;

	for (iter = 0; iter < 100; iter++)
	{
		TIMESTAMP(thread_id, "START", iter);
		for (cnt = 0; cnt < thread_id * ITERCOUNT; cnt++)
			asm volatile("":::"memory");
		TIMESTAMP(thread_id, "END", iter);

		TIMESPEC_ADD(next_act, period);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_act, NULL);
	}

	return NULL;
}

int main()
{
	long i;
	pthread_t tid[THREADS];

	clock_gettime(CLOCK_REALTIME, &t0); /* "zero" time */
	clock_gettime(CLOCK_MONOTONIC, &tx0);

	/* create threads */
	for (i = 0; i < THREADS; i++) {
		if (pthread_create(&tid[i], NULL, thread_func, (void *)(i+1))){
			perror("Error: pthread_create");
			return 1;
		}
	}

	/* wait for threads to finish */
	for (i = 0; i < THREADS; i++)
		pthread_join(tid[i], NULL);

	return 0;
}

/* Example run: (on single processor system !!!)
$ gcc periodic-tasks-1.c -pthread -Wall
$ ./a.out
[00:000140] Thread 3: START iter=0
[00:000460] Thread 2: START iter=0
[00:008245] Thread 1: START iter=0
[00:246640] Thread 1: END iter=0
[00:418966] Thread 2: END iter=0
[00:500457] Thread 1: START iter=1
[00:522505] Thread 3: END iter=0
[00:613793] Thread 1: END iter=1
[01:000499] Thread 2: START iter=1
[01:004525] Thread 1: START iter=2
...
*/
