#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

static int act[2] = {0, 0};

static void signal_handler(int sig, siginfo_t *info, void *context)
{
	int id = 1;
	if (info != NULL && info->si_code == SI_TIMER)
		id = info->si_value.sival_int;

	printf("Timer %d [%d]\n", id, ++act[id-1]);
}

int main()
{
	timer_t timer1, timer2;
	struct sigevent event;
	struct itimerspec period;
	struct sigaction act;

	/* handle SIGUSR1 with signal_handler */
	act.sa_sigaction = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO | SA_NODEFER;
	sigaction(SIGUSR1, &act, NULL);

	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGUSR1;
	event.sigev_value.sival_int = 1;

	timer_create(CLOCK_REALTIME, &event, &timer1);

	event.sigev_value.sival_int = 2;
	timer_create(CLOCK_REALTIME, &event, &timer2);

	period.it_value.tv_sec = period.it_interval.tv_sec = 1;
	period.it_value.tv_nsec = period.it_interval.tv_nsec = 0;
	timer_settime(timer1, 0, &period, NULL);

	period.it_value.tv_sec = period.it_interval.tv_sec = 2;
	timer_settime(timer2, 0, &period, NULL);

	for (int i = 0; i < 10; i++)
		pause(); //čekaj da te signal probudi

	return 0;
}
