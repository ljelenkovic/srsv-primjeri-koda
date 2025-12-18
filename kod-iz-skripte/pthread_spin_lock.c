/*! Simple semaphore example with sem_wait/sem_post interface */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*--- macro that simplifies handling errors with function calls ------------- */
#define ACT_WARN	0
#define ACT_STOP	1
#define CALL(ACT,FUNC,...)                                \
do {                                                      \
    if (FUNC(__VA_ARGS__)) {                              \
        perror(#FUNC);                                    \
        if (ACT == ACT_STOP)                              \
            exit(1);                                      \
    }                                                     \
} while(0)

/* for example, instead of:
 *    if (pthread_create(&t1, NULL, worker, (void *) p)) {
 *        perror("pthread_create");
 *        exit(-1);
 *    }
 * use:
 *    CALL(ACT_STOP, pthread_create, &t1, NULL, worker, (void *) 1);
 *
 *--------------------------------------------------------------------------- */

#define THREADS  6
static pthread_spinlock_t lock;
static int work_in_progress = 1;

static void *worker(void *p)
{
	long id = (long) p;
	struct timespec t = { id, 0 };

	printf("Thread %ld starting\n", id);

	while (work_in_progress) {
		CALL(ACT_STOP, pthread_spin_lock, &lock);

		printf("Thread %ld inside C.S.\n", id);

		CALL(ACT_WARN, nanosleep, &t, NULL);

		printf("Thread %ld leaving C.S.\n", id);

		CALL(ACT_STOP, pthread_spin_unlock, &lock);

		CALL(ACT_WARN, nanosleep, &t, NULL);
	}

	printf("Thread %ld exiting\n", id);

	return NULL;
}

int main()
{
	long i;
	pthread_t thr[THREADS];
	struct timespec t = { 50, 0 };

	CALL(ACT_STOP, pthread_spin_init, &lock, PTHREAD_PROCESS_PRIVATE);

	for (i = 0; i < THREADS; i++)
		CALL(ACT_STOP, pthread_create, &thr[i], NULL, worker, (void *) i+1);

	CALL(ACT_WARN, nanosleep, &t, NULL);

	work_in_progress = 0;

	for (i = 0; i < THREADS; i++)
		CALL(ACT_WARN, pthread_join, thr[i], NULL);

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
