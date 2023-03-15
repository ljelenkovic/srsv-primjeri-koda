/*! Synchronization example with monitor on "old bridge" problem */

#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

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


#define THREADS  20

/* mutex and conditional variables, statically initialized */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_n = PTHREAD_COND_INITIALIZER;
static pthread_cond_t n_s = PTHREAD_COND_INITIALIZER;
static pthread_cond_t *cq[2] = { &s_n, &n_s };

struct CarInfo {
	int id;
	int dir;
};
static char *sdir[] = { "south", "north" };

static int cars_on_bridge = 0;
static int dir_on_bridge = -1; /* 0: S->N; 1:N->S; -1:bridge empty */

void *car_thread ( void *p )
{
	struct CarInfo *car = p;
	struct timespec t = { 5, 0 };

	CALL ( ACT_STOP, pthread_mutex_lock, &m );

	while ( cars_on_bridge > 2 ||
            ( dir_on_bridge != -1 && dir_on_bridge != car->dir ) )
		CALL ( ACT_STOP, pthread_cond_wait, cq[car->dir], &m );

	cars_on_bridge++;
	dir_on_bridge = car->dir;

	printf("Car %2d  on bridge, going %s, on bridge %d car(s) (to %s)\n",
		car->id, sdir[car->dir], cars_on_bridge, sdir[dir_on_bridge] );

	CALL ( ACT_STOP, pthread_mutex_unlock, &m );

	CALL ( ACT_WARN, nanosleep, &t, NULL );

	CALL ( ACT_STOP, pthread_mutex_lock, &m );

	cars_on_bridge--;

	printf ( "Car %2d off bridge, going %s, on bridge %d car(s) (to %s)\n",
		car->id, sdir[car->dir], cars_on_bridge, sdir[dir_on_bridge] );

	if ( cars_on_bridge > 0 ) {
		CALL ( ACT_STOP, pthread_cond_signal, cq[car->dir] );
	}
	else {
		dir_on_bridge = -1;
		CALL ( ACT_STOP, pthread_cond_broadcast, cq[1 - car->dir] );
	}

	CALL ( ACT_STOP, pthread_mutex_unlock, &m );

	free (car);

	return NULL;
}

int main ()
{
	pthread_t thr_id;
	pthread_attr_t attr;
	int i;
	struct CarInfo *car;
	struct timespec t = { 2, 0 };

	CALL ( ACT_STOP, pthread_attr_init, &attr );
	CALL ( ACT_STOP, pthread_attr_setdetachstate, &attr, PTHREAD_CREATE_DETACHED );

	for ( i = 0; i < THREADS; i++ ) {
		car = malloc ( sizeof(struct CarInfo) );
		car->id = i+1;
		car->dir = rand() & 1;

		printf( "New car %d arrived from %s\n", i+1, sdir[1 - car->dir] );

		CALL ( ACT_STOP, pthread_create, &thr_id, &attr, car_thread, (void *) car );

		CALL ( ACT_WARN, nanosleep, &t, NULL );
	}

	return 0;
}

/* Example run:
$ gcc pthread_mutex.c -pthread -Wall
$ ./a.out
New car 1 arrived from south
Car  1  on bridge, going north, on bridge 1 car(s) (to north)
New car 2 arrived from north
New car 3 arrived from south
Car  3  on bridge, going north, on bridge 2 car(s) (to north)
Car  1 off bridge, going north, on bridge 1 car(s) (to north)
New car 4 arrived from south
Car  4  on bridge, going north, on bridge 2 car(s) (to north)
New car 5 arrived from south
Car  5  on bridge, going north, on bridge 3 car(s) (to north)
Car  3 off bridge, going north, on bridge 2 car(s) (to north)
New car 6 arrived from south
Car  6  on bridge, going north, on bridge 3 car(s) (to north)
Car  4 off bridge, going north, on bridge 2 car(s) (to north)
New car 7 arrived from north
Car  5 off bridge, going north, on bridge 1 car(s) (to north)
New car 8 arrived from north
Car  6 off bridge, going north, on bridge 0 car(s) (to north)
Car  2  on bridge, going south, on bridge 1 car(s) (to south)
...
*/
