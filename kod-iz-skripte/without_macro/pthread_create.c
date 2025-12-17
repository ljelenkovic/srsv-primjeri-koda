/*! Simple pthread_create example */

#include <stdio.h>
#include <pthread.h>

static void *new_thread ( void *p )
{
	long *n = p;
	long num = *n;

	printf ( "In thread %ld\n", num );

	*n = 0; /* exit status = 0 (success) */

	return n; //or pthread_exit (n);
}

int main ()
{
	pthread_t t1, t2;
	long n1 = 1, n2 = 5, *status1, *status2;

	/* create threads */
	pthread_create ( &t1, NULL, new_thread, (void *) &n1 );
	pthread_create ( &t2, NULL, new_thread, (void *) &n2 );

	/* wait until created threads finishes */
	pthread_join ( t1, (void *) &status1 );
	pthread_join ( t2, (void *) &status2 );

	printf ( "Collected statuses: %ld %ld\n", *status1, *status2 );

	return 0;
}

/* Example run:
$ gcc pthread_create.c -pthread -Wall
$ ./a.out
In thread 5
In thread 1
Collected statuses: 0 0
$
*/
