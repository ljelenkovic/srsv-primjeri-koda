#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#define BASE_PRIO 6

typedef struct {
	int T;
	int t0;
	int c;
	int x;
} task;

typedef struct {
	int change_count;
	int avg_react;
	int max_react;
	int not_done;
} stat;

// ulazi
task ulaz[] = { //{.T, .t0, .C, .x} x=1 stvarni ulaz, x=0 popuna (LAB2)
	//ovaj redak svake sekunde
	{ 1000, 0, 30, 1}, {1000, 400, 30, 1}, {1000, 700, 30, 1}, //a1-a3

	//samo jedan od ovih pet redaka svake sekunde
	{ 5000,  100, 50, 1}, {5000,  500, 50, 1}, {5000,  800, 50, 1}, //b1-b3
	{ 5000, 1100, 50, 1}, {5000, 1500, 50, 1}, {5000, 1800, 50, 1}, //b4-b6
	//{ 5000, 2100, 50, 1}, {5000, 2500, 50, 1}, {5000, 2800, 50, 1}, //b7-b9
	//{ 5000, 3100, 50, 1}, {5000, 3500, 50, 1}, {5000, 3800, 50, 1}, //b9-b12
	//{ 5000, 4100, 50, 1}, {5000, 4500, 50, 1}, {5000, 4800, 50, 1}, //b13-b15

	//samo jedan od ovih 20 redaka svake sekunde
	{20000,   900,  50, 1},  //c1
	{20000,  1900, 150, 1},  //c2
	{20000,  2900,  50, 1},  //c3
	//{20000,  3900, 150, 1},  //c4
	//{20000,  4900,  50, 1},  //c5
	//{20000,  5900, 150, 1},  //c6
	//{20000,  6900,  50, 1},  //c7
	//{20000,  7900, 150, 1},  //c8
	//{20000,  8900,  50, 1},  //c9
	//{20000,  9900, 150, 1},  //c10
	//{20000, 10900,  50, 1},  //c11
	//{20000, 11900, 150, 1},  //c12
	//{20000, 12900,  50, 1},  //c13
	//{20000, 13900, 150, 1},  //c14
	//{20000, 14900,  50, 1},  //c15
	//{20000, 15900, 150, 1},  //c16
	//{20000, 16900,  50, 1},  //c17
	//{20000, 17900, 150, 0},  //c18-samo popunjavanje tablice
	//{20000, 18900, 150, 0},  //c19-samo popunjavanje tablice
	//{20000, 19900, 150, 0}   //c20-samo popunjavanje tablice
};
#define input_count sizeof(ulaz)/sizeof(ulaz[0])

pthread_t input_thread[input_count];

// varijable za komunikaciju između dretvi
// sve tri su atomic da se izbjegne race condition
atomic_int change[input_count];
atomic_int done[input_count];
atomic_int react_time[input_count];

// polje za statistiku
stat stats[input_count];

// broj iteracija za 10 ms
unsigned int iter10ms = 0;

// vrijeme početka programa
int start = 0;

// funkcija koja vraća vrijeme u milisekundama
unsigned int get_time_ms() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

// funkcija za čekanje 10 ms
void wait10ms() {
	for(volatile int i=0; i<iter10ms; i++);
}

// izračun broja iteracija za 10 ms
void set_iter() {
	iter10ms = 1000;
	int done = 0;

	while(!done) {
		int t0 = get_time_ms();
		wait10ms();
		int t1 = get_time_ms();

		if(t1 - t0 >= 10) {
			done = 1;
		} else {
			iter10ms *= 10;
		}
	}
}

// čekanje s korištenjem wait10ms() funkcije
void iter_wait(int time) {
	for(int i=0; i<(time/10); i++) {
		wait10ms();
	}
}

// funkcija za zaustavljanje
void stop() {

	for(int i=0; i<input_count; i++) {
		pthread_cancel(input_thread[i]);
	}

	int change_count=0;
	int avg_react = 0;
	int max_react = 0;
	int not_done = 0;

	// ispiši statistiku za svaki ulaz
	for(int i=0; i<input_count; i++) {
		printf(" ======================== Ulaz %d ======================== \n", i);
		printf("broj promjena stanja: %d\n", stats[i].change_count);
		printf("prosječno vrijeme reakcije na promjenu stanja: %d ms\n", stats[i].avg_react);
		printf("maksimalno vrijeme reakcije na promjenu stanja: %d ms\n", stats[i].max_react);
		printf("broj neobrađenih događaja: %d\n", stats[i].not_done);

		change_count += stats[i].change_count;
		avg_react += stats[i]. change_count + stats[i].avg_react;
		if(stats[i].max_react > max_react) {
			max_react = stats[i].max_react;
		}
		not_done += stats[i].not_done;
	}

	// ispiši statistiku za sve
	avg_react = avg_react / change_count;
	printf(" ======================== Ukupno ======================== \n");
	printf("broj promjena stanja: %d\n", change_count);
	printf("prosječno vrijeme reakcije na promjenu stanja: %d ms\n", avg_react);
	printf("maksimalno vrijeme reakcije na promjenu stanja: %d ms\n", max_react);
	printf("broj neobrađenih događaja: %d\n", not_done);

	exit(0);
}

// funkcija za inicijalizaciju
void init() {

	// postavi raspoređivanje na SCHED_FIFO
	
	struct sched_param params;
	params.sched_priority = BASE_PRIO;
	if(sched_setscheduler(0, SCHED_FIFO, &params) < 0) {
		fprintf(stderr, "sched_setscheduler() returned an error\n");
		exit(-1);
	}

	//mapiraj signal za prekid simulacije
	struct sigaction act1;
	act1.sa_handler = stop;
	sigaction(SIGINT, &act1, NULL);

	// inicijaliziraj varijable
	for(int i=0; i<input_count; i++) {
		stats[i].change_count = 0;
		stats[i].avg_react = 0;
		stats[i].max_react = 0;
		stats[i].not_done = 0;

		change[i] = ATOMIC_VAR_INIT(0);
		done[i] = ATOMIC_VAR_INIT(0);
		react_time[i] = ATOMIC_VAR_INIT(0);
	}

	// izračunaj broj iteracija za 10 ms
	set_iter();

	start = get_time_ms();
}

// funkcija za dretve za ulaze
void* input(void* i) {

	int id = *((int*) i);
	free(i);

	// pročitaj vrijeme i period
	int t = ulaz[id].t0;
	int T = ulaz[id].T;

	while(1) {
		// odgodi do t
		while(get_time_ms() - start < t) {
			usleep(10 * 1000);
		}

		// resetiraj done
		atomic_store(&done[id], 0);

		// zabilježi trenutak promjene stanja
		atomic_store(&react_time[id], get_time_ms());		

		// promjeni stanje
		printf("Ulaz %d: promjena stanja\n", id);
		atomic_store(&change[id], 1);
		stats[id].change_count++;
		
		// čekaj dok obrada nije gotova
		while(!atomic_load(&done[id]) && t + T > (get_time_ms() - start)) {
			usleep(10 * 1000);
		}

		// postavi da nema više promjene jer je obrađena ili je vrijeme isteklo
		// ako ju upravljač nije do sada vidio, onda je propustio priliku
		atomic_store(&change[id], 0);

		// updateaj statistiku
		if(atomic_load(&done[id])) {
			
			// updateaj statistiku
			int n = stats[id].change_count - stats[id].not_done; 
			stats[id].avg_react = (stats[id].avg_react * (n-1) / n) + (atomic_load(&react_time[id]) / n);
			if(atomic_load(&react_time[id]) > stats[id].max_react) {
				stats[id].max_react = atomic_load(&react_time[id]);
			}
			
			printf("Ulaz %d: gotova obrada, reakcija %d ms\n", id, atomic_load(&react_time[id]));
		} else {
			// označi u statistici da nije završio
			stats[id].not_done++;

			printf("Ulaz %d: nije obrađen\n", id);
		}

		// povečaj t za period T
		t += T;
	}
}

int main() {

	init();

	for(int i=0; i<input_count; i++) {
		int* id1 = (int*) malloc(sizeof(int));
		*id1 = i;
		pthread_create(&input_thread[i], NULL, input, (void*) id1);	
	}

	while(1) {
		for(int i=0; i<input_count; i++) {
			if(atomic_load(&change[i])) {

				// zabilježi trenutak reakcije
				atomic_store(&react_time[i], get_time_ms() - atomic_load(&react_time[i]));

				printf("Upr: započinje obrada ulaza %d\n", i);

				// simuliraj obradu
				iter_wait(ulaz[i].c);

				if(atomic_load(&change[i])) {
					// označi kraj obrade ako ju nije propustio
					atomic_store(&done[i], 1);
					atomic_store(&change[i], 0);	
					printf("Upr %d: završena obrada ulaza\n", i);
				}
			}
		}
	}

	for(int i=0; i<input_count; i++) {
		pthread_cancel(input_thread[i]);
	}

	return 0;
}
