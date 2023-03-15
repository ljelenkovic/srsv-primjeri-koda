#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

void signal_handler ( int sig, siginfo_t *info, void *context )
{
	int j;

	printf ( "[Signal handler] processing started for signal %d", sig );
	if ( info != NULL && info->si_code == SI_QUEUE )
		printf ( " [.sival_int = %d / .sival_ptr = %p]\n",
			info->si_value.sival_int, info->si_value.sival_ptr );
	else
		printf ( "\n" );

	for ( j = 1; j <= 5; j++ ) {
		printf("[Signal handler] processing signal %d: %d/5\n", sig, j);
		sleep(1);
	}

	printf("[Signal handler] processing completed for signal %d\n", sig);
}

static pid_t parent;
static pthread_t main_thread;

void *signal_me ( void *p )
{
	union sigval v;

	sigset_t sigmask;
	sigemptyset ( &sigmask );
	sigaddset ( &sigmask, SIGUSR1 );
	sigaddset ( &sigmask, SIGUSR2 );
	pthread_sigmask ( SIG_BLOCK, &sigmask, NULL );

	v.sival_ptr = NULL;

	sleep (1);
	printf ( "[Signaler] sending SIGUSR1 (to main thread)\n" );
	pthread_kill ( main_thread, SIGUSR1 );

	sleep (3);
	printf ( "[Signaler] sending SIGUSR1 (to process)\n" );
	v.sival_int = 1;
	sigqueue ( parent, SIGUSR1, v );

	sleep (10);
	printf ( "[Signaler] sending SIGUSR2 (to process)\n" );
	v.sival_int = 2;
	sigqueue ( parent, SIGUSR2, v );

	return NULL;
}

int main ()
{
	struct sigaction act;
	sigset_t sigmask;
	pthread_t signaler;
	siginfo_t info;

	parent = getpid();
	main_thread = pthread_self();

	/* create thread that will raise signals */
	pthread_create ( &signaler, NULL, signal_me, NULL );

	/* handle SIGUSR1 with signal_handler */
	act.sa_sigaction = signal_handler;
	sigemptyset ( &act.sa_mask );
	sigaddset ( &act.sa_mask , SIGUSR1 );
	sigaddset ( &act.sa_mask , SIGUSR2 );
	act.sa_flags = SA_SIGINFO;
	sigaction ( SIGUSR1, &act, NULL );

	/* mask SIGUSR2 (sigwait used to catch it) */
	sigemptyset ( &sigmask );
	sigaddset ( &sigmask, SIGUSR2 );
	pthread_sigmask ( SIG_BLOCK, &sigmask, NULL );

	printf ( "[Main thread] waiting for SIGUSR2\n" );

	while ( sigwaitinfo ( &sigmask, &info ) == -1 )
		perror ( "sigwaitinfo" ); /* interrupted with SIGUSR1? */

	printf ( "[Main thread] caught signal %d", info.si_signo );
	if ( info.si_code == SI_QUEUE )
		printf ( " [.sival_int = %d / .sival_ptr = %p]\n",
			info.si_value.sival_int, info.si_value.sival_ptr );
	else
		printf ( "\n" );

	pthread_join ( signaler, NULL );

	return 0;
}
