#include <stdio.h>
#include <time.h>

#define TIMESPEC_SUB(A,B) /* A -= B */ \
do {                                   \
    (A).tv_sec -= (B).tv_sec;          \
    (A).tv_nsec -= (B).tv_nsec;        \
    if ( (A).tv_nsec < 0 ) {           \
        (A).tv_sec--;                  \
        (A).tv_nsec += 1000000000;     \
    }                                  \
} while (0)

//printf with time prefix
#define PRINTF(format, ...)                                 \
do {                                                        \
    struct timespec _t_;                                    \
    clock_gettime(CLOCK_MONOTONIC, &_t_);                   \
    TIMESPEC_SUB(_t_, t0);                                  \
    printf("[%03ld:%06ld] " format,              \
        _t_.tv_sec % 100, _t_.tv_nsec/1000, ##__VA_ARGS__); \
} while (0)

// sleep in milliseconds
int msleep(int msec)
{
	struct timespec t;
	t.tv_sec = msec / 1000;
	t.tv_nsec = (msec % 1000) * 1000000;
	return clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
}

struct timespec t0; //PRINTF

int main()
{
	FILE *fp;
	int broj;

	// struct timespec t1, t2;
	// clock_gettime(CLOCK_REALTIME, &t1);
	// FILE *fp = fopen("tipka.txt", "r");
	// int broj = fgetc(fp);
	// fclose(fp);
	// clock_gettime(CLOCK_REALTIME, &t2);

	// printf("%ld:%ld\n%ld:%ld\n", t2.tv_sec, t2.tv_nsec, t1.tv_sec, t1.tv_nsec);


	fp = fopen("tipka.txt", "r");
	broj = fgetc(fp);
	fclose(fp);

	PRINTF("Stanje prije prve petlje: %c\n", broj);

	while (broj == '0') {
		msleep(1);
		fp = fopen("tipka.txt", "r");
		broj = fgetc(fp);
		fclose(fp);
	}

	PRINTF("Stanje nakon prve petlje: %c\n", broj);

	while (broj != '0') {
		msleep(1);
		fp = fopen("tipka.txt", "r");
		broj = fgetc(fp);
		fclose(fp);
	}

	PRINTF("Stanje nakon druge petlje: %c\n", broj);

	return 0;
}
