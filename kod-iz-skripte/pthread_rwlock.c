/*! Simple example with read/write locks */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*--- macro that simplifies handling errors with function calls ------------- */
#define ACT_WARN	0
#define ACT_STOP	1
#define CALL(ACT,FUNC,...)                                \
do {                                                      \
    if (FUNC(__VA_ARGS__)) {                         \
        perror(#FUNC);                                 \
        if (ACT == ACT_STOP)                            \
            exit (1);                                     \
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
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

static void *worker(void *p)
{
	long id = (long) p;
	struct timespec t = { 2, 0 };

	if (id & 1) {
		printf("[%ld] Reader started\n", id);
		CALL(ACT_STOP, pthread_rwlock_rdlock, &rwlock);
		printf("[%ld] Read lock acquired\n", id);
	}
	else {
		printf("[%ld] Writter started\n", id);
		CALL(ACT_STOP, pthread_rwlock_wrlock, &rwlock);
		printf("[%ld] Write lock acquired\n", id);
	}

	CALL(ACT_WARN, nanosleep, &t, NULL);

	if (id & 1)
		printf("[%ld] Releasing read lock\n", id);
	else
		printf("[%ld] Releasing write lock\n", id);
	CALL(ACT_STOP, pthread_rwlock_unlock, &rwlock);

	return NULL;
}

int main ()
{
	long i;
	pthread_t thr[THREADS];
	struct timespec t = { 1, 0 };

	for (i = 0; i < THREADS; i++) {
		CALL(ACT_STOP, pthread_create, &thr[i], NULL, worker, (void *) i+1);
		t.tv_sec = i;
		CALL(ACT_WARN, nanosleep, &t, NULL);
	}

	for (i = 0; i < THREADS; i++)
		CALL(ACT_WARN, pthread_join, thr[i], NULL);

	return 0;
}

/* Example run:
$ gcc pthread_rwlock.c -pthread -Wall
$ ./a.out
[1] Reader started
[1] Read lock acquired
[2] Writter started
[3] Reader started
[3] Read lock acquired
[1] Releasing read lock
[3] Releasing read lock
[2] Write lock acquired
[4] Writter started
[2] Releasing write lock
[4] Write lock acquired
[5] Reader started
[4] Releasing write lock
[5] Read lock acquired
[5] Releasing read lock
[6] Writter started
[6] Write lock acquired
[6] Releasing write lock
$
*/
