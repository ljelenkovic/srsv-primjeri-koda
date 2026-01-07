#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

static int act[2] = { 0, 0 };

static void alarm_thread(union sigval val)
{
	printf("Timer %d [%d]\n", val.sival_int, ++act[val.sival_int-1]);

	return;
}

int main ()
{
	timer_t timer1, timer2;
	struct sigevent event;
	struct itimerspec period;
	struct timespec t;

	event.sigev_notify = SIGEV_THREAD;
	event.sigev_notify_function = alarm_thread;
	event.sigev_notify_attributes = NULL;

	event.sigev_value.sival_int = 1;
	timer_create(CLOCK_REALTIME, &event, &timer1);

	event.sigev_value.sival_int = 2;
	timer_create(CLOCK_REALTIME, &event, &timer2);

	period.it_value.tv_sec = period.it_interval.tv_sec = 1;
	period.it_value.tv_nsec = period.it_interval.tv_nsec = 0;
	timer_settime(timer1, 0, &period, NULL);

	period.it_value.tv_sec = period.it_interval.tv_sec = 2;
	timer_settime(timer2, 0, &period, NULL);

	t.tv_sec = 10;
	t.tv_nsec = 0;
	nanosleep(&t, NULL);

	return 0;
}
