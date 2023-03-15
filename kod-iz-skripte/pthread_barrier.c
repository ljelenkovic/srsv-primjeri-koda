/*! Simple barrier synchronization example */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*--- macro that simplifies handling errors with function calls ------------- */
#define ACT_WARN	0
#define ACT_STOP	1
#define CALL(ACT,FUNC,...)                                \
do {                                                      \
    if ( FUNC ( __VA_ARGS__ ) ) {                         \
        perror ( #FUNC );                                 \
        if ( ACT == ACT_STOP )                            \
            exit (1);                                     \
    }                                                     \
} while(0)

/* for example, instead of:
 *    if ( pthread_create ( &t1, NULL, worker, (void *) p ) ) {
 *        perror ( "pthread_create" );
 *        exit ( -1 );
 *    }
 * use:
 *    CALL ( ACT_STOP, pthread_create, &t1, NULL, worker, (void *) 1 );
 *
 *--------------------------------------------------------------------------- */

#define THREADS  6
#define BARRIER  3

static pthread_barrier_t barrier;

static void *worker ( void *p )
{
	long id = (long) p;
	int retval;

	CALL ( ACT_WARN, pthread_detach, pthread_self() );

	printf ( "Thread %ld at barrier\n", id );

	retval = pthread_barrier_wait ( &barrier );
	if ( retval == 0 || retval == PTHREAD_BARRIER_SERIAL_THREAD ) {
		printf ( "Thread %ld passed barrier\n", id );
	}
	else {
		perror ( "pthread_barrier_wait" );
		exit ( -1 );
	}

	return NULL;
}

int main ()
{
	long i;
	pthread_t tmp;
	struct timespec t = { 2, 0 };

	CALL ( ACT_STOP, pthread_barrier_init, &barrier, NULL, BARRIER );

	for ( i = 0; i < THREADS; i++ ) {
		CALL ( ACT_STOP, pthread_create, &tmp, NULL, worker, (void *) i+1 );
		CALL ( ACT_WARN, nanosleep, &t, NULL );
	}

	CALL ( ACT_WARN, nanosleep, &t, NULL );

	return 0;
}

/* Example run:
$ gcc pthread_barrier.c -pthread -Wall
$ ./a.out
Thread 1 at barrier
Thread 2 at barrier
Thread 3 at barrier
Thread 1 passed barrier
Thread 2 passed barrier
Thread 3 passed barrier
Thread 4 at barrier
Thread 5 at barrier
Thread 6 at barrier
Thread 4 passed barrier
Thread 5 passed barrier
Thread 6 passed barrier
$
*/
