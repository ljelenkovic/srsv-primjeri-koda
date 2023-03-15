/*! Simple thread specific memory example */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <malloc.h>
#include <time.h>

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


/* dummy structures */
typedef long ThrStat;
typedef long ThrBuffer;

/* global variables, shared among threads */
static pthread_key_t thr_stat, thr_buffer;

/* function prototypes */
static void *worker ( void *x );
static void func ();
static void free_data ( void *x );

int main()
{
	pthread_t t1, t2;

	/* main thread – initialization of ‘keys’, basis for thread specific data */
	CALL ( ACT_STOP, pthread_key_create,  &thr_stat, free_data );
	CALL ( ACT_STOP, pthread_key_create,  &thr_buffer, free_data );
	/* initially, value NULL is associated with each key for all threads */

	/* create threads */
	CALL ( ACT_STOP, pthread_create, &t1, NULL, worker, (void *) 1 );
	CALL ( ACT_STOP, pthread_create, &t2, NULL, worker, (void *) 2 );

	/* wait until created threads finishes */
	CALL ( ACT_WARN, pthread_join, t1, NULL );
	CALL ( ACT_WARN, pthread_join, t2, NULL );

	return 0;
}

/* worker thread - initialization */
static void *worker ( void *x )
{
	ThrStat *stat;
	ThrBuffer *buffer;
	struct timespec t = { 0, 0 };

	stat = malloc ( sizeof(ThrStat) );
	buffer = malloc ( sizeof(ThrBuffer) );
	if ( !stat || !buffer ) {
		perror ( "malloc" );
		exit (1);
	}

	*stat = (long) x;
	*buffer = *stat * 10;

	/* associate stat with key thr_stat for current thread only */
	CALL ( ACT_STOP, pthread_setspecific, thr_stat, stat );
	CALL ( ACT_STOP, pthread_setspecific, thr_buffer, buffer );

	t.tv_sec = 2 * (long) x;
	CALL ( ACT_WARN, nanosleep, &t, NULL );

	func();

	return NULL;
}

/* worker thread – in some function, e.g. in signal handler */
static void func ()
{
	ThrStat *s;
	ThrBuffer *b;

	//get data associated with keys
	s = pthread_getspecific ( thr_stat );
	b = pthread_getspecific ( thr_buffer );

	if ( !s || !b ) {
		fprintf ( stderr, "ERROR: pthread_getspecific returned NULL" );
		return;
	}

	/* use ‘s’ and ‘b’ */
	printf ( "[thread] stat=%ld, buffer=%ld\n", *s, *b );
}

/* release thread specific data, called on thread termination */
static void free_data ( void *x )
{
	printf("Releasing thread specific data at %p\n", x);
	if (x)
		free(x);
}

/* Example run:
$ gcc pthread_specific.c -pthread -Wall
$ ./a.out
[thread] stat=1, buffer=10
Releasing thread specific data at 0x7f353c0008c0
Releasing thread specific data at 0x7f353c0008e0
[thread] stat=2, buffer=20
Releasing thread specific data at 0x7f35440008c0
Releasing thread specific data at 0x7f35440008e0
$
*/
