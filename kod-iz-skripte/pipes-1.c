#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define PIPE_NAME  "/tmp/pipe_example"
#define MAX_BUF    50

void reader ()
{
	int fd, i, sz;
	char buffer[MAX_BUF];

	fd = open ( PIPE_NAME, O_RDONLY ); /* wait for writer's "open" */
	if ( fd == -1 ) {
		perror ( "reader:open" );
		exit (1);
	}
	printf ( "Reader side opened\n" );

	i = 0;
	memset ( buffer, 0, MAX_BUF * sizeof(char) );
	while( (sz = read ( fd, &buffer[i], MAX_BUF * sizeof(char) - i) ) > 0 )
		i += sz;
	printf ( "Data received: %s\n", buffer );

	exit (0);
}

int main ()
{
	int fd, i;
	char *message[] = { "abcd", "ABC", "12345", "X", NULL };

	if ( mkfifo ( PIPE_NAME, S_IWUSR | S_IRUSR ) )
		perror ( "writer:mkfifo" ); /* maybe its already created */

	if ( fork() == 0 )
		reader ();

	sleep (1); /* reader will also wait! */
	fd = open ( PIPE_NAME, O_WRONLY ); /* wait for reader's "open" */
	if ( fd == -1 ) {
		perror ( "writer:open" );
		return 1;
	}
	printf ( "Writer side opened\n" );

	for ( i = 0; message[i] != NULL; i++ )
		write ( fd, message[i], strlen (message[i]) );
	close (fd); /* read on pipe will return 0 after pipe was emptied */

	wait (NULL);

	unlink ( PIPE_NAME );

	return 0;
}
