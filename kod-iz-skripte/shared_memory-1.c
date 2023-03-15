#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>

struct shared {
	int a, b;
};

#define SM_NAME  "/sm_example_name"  /* created in /dev/shm/ */
#define SM_SIZE  sizeof (struct shared)
#define PROC_NUM 10

void process ( int app_id )
{
	int id, tmp;
	struct shared *x;

	sleep ( app_id ); /* sync by sleep */

	/* create/open shared memory object; map it to memory */
	id = shm_open ( SM_NAME, O_CREAT | O_RDWR, 00600 );
	if ( id == -1 || ftruncate ( id, SM_SIZE ) == -1) {
		perror ( "shm_open/ftruncate" );
		exit(1);
	}
	x = mmap ( NULL, SM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, id, 0 );
	if ( x == (void *) -1) {
		perror ( "mmap" );
		exit(1);
	}
	close ( id );

	if ( app_id == 0 ) {
		x->a = 0;
		x->b = 1;
	} else {
		tmp = x->a + x->b;
		x->a = x->b;
		x->b = tmp;
	}
	printf ( "[%d] %d\n", app_id+1, x->b );

	/* last process should delete shared memory */
	if ( app_id == PROC_NUM-1 ) {
		munmap ( x, SM_SIZE );
		shm_unlink ( SM_NAME );
	}
}

int main ( void )
{
	int i;

	for ( i = 0; i < PROC_NUM; i++ )
		if ( !fork() ) {
			process (i);
			exit (0);
		}
	for ( i = 0; i < PROC_NUM; i++ )
		wait (NULL);

	return 0;
}
