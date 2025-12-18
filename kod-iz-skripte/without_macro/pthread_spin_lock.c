/*! Simple semaphore example with sem_wait/sem_post interface */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#define THREADS  6
static pthread_spinlock_t lock;
static int work_in_progress = 1;

static void *worker(void *p)
{
	long id = (long) p;
	struct timespec t = {id, 0};

	printf("Thread %ld starting\n", id);

	while (work_in_progress) {
		pthread_spin_lock(&lock);

		printf("Thread %ld inside C.S.\n", id);

		nanosleep(&t, NULL);

		printf("Thread %ld leaving C.S.\n", id);

		pthread_spin_unlock(&lock);

		nanosleep(&t, NULL);
	}

	printf("Thread %ld exiting\n", id);

	return NULL;
}

int main()
{
	long i;
	pthread_t thr[THREADS];
	struct timespec t = {50, 0};

	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	for (i = 0; i < THREADS; i++)
		pthread_create(&thr[i], NULL, worker, (void *) i+1);

	nanosleep(&t, NULL);

	work_in_progress = 0;

	for (i = 0; i < THREADS; i++)
		pthread_join(thr[i], NULL);

	return 0;
}

/* Example run:
$ gcc pthread_spin_lock.c -pthread -Wall
$ ./a.out
Thread 6 starting
Thread 6 inside C.S.
Thread 5 starting
Thread 4 starting
Thread 3 starting
Thread 2 starting
Thread 1 starting
Thread 6 leaving C.S.
Thread 2 inside C.S.
Thread 2 leaving C.S.
Thread 1 inside C.S.
Thread 1 leaving C.S.
Thread 5 inside C.S.
...
*/
