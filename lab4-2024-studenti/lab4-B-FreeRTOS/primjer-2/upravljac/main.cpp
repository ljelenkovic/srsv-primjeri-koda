#include <iostream>
#include <stdio.h>
#include <time.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdbool.h>
#include <csignal>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"


//skracivanje roka za obradu
#define K 1 //koeficijent koji daje najveći broj neobradjenih ulaza
#define X -100 //pomak

#define BROJ_UPRAVLJACA 1

#define BROJ_A 2
#define BROJ_B 1
#define BROJ_C 1
#define BROJ_ULAZA BROJ_A+BROJ_B+BROJ_C  //A+B+C
#define SIMULATOR_CEKANJE 10
#define PREKID_CEKANJE 100

#define VELICINA_REDA 10
#define POMAK_PERIODA_PREKORACENJA 10

#define BROJ_ITERACIJA BROJ_ULAZA

bool gotovo[BROJ_ULAZA] = {false};
int potroseno_perioda[BROJ_ULAZA] = {0};



int redosljed=0;
int trenutni_zad[] = {0, 0, 0};  //indeks zadatka, zadnji poznati zadatak
int brojac_simulator[BROJ_ULAZA] = {0};


#define HIGH 1
#define LOW 0

#define SIMULATOR_IN_GPIO_A 11 
#define SIMULATOR_OUT_GPIO_A 12 

#define SIMULATOR_IN_GPIO_B 17 
#define SIMULATOR_OUT_GPIO_B 18 

#define SIMULATOR_IN_GPIO_C 1 
#define SIMULATOR_OUT_GPIO_V 2 

#define SIMULATOR_IN_GPIO_D 36 
#define SIMULATOR_OUT_GPIO_D 37 

gpio_num_t m_pinNumber_in[BROJ_ULAZA];
gpio_num_t m_pinNumber_out[BROJ_ULAZA];


const char *LogName = "upravljac";


typedef struct input_data {
    int T, t0, C, x;
};

input_data ulaz[] = { //{.T, .t0, .C, .x} x=1 stvarni ulaz, x=0 popuna (LAB2)
//ovaj redak svake sekunde
{ 1000, 0, 70, 1}, {1000, 300, 70, 1}, {3000, 600, 150, 1}, {3000, 600, 150, 1}};

//tablica reaspoređivanja
int red[] = { 0, 1, -1, 0, 1, -1, 0, 1, 2, -1};  //za svaki prekid svakih 100ms

//int zadA[] = {1, 2, 3};
//int zadB[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
//int zadC[] = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37};
int zad[] = {1, 2, 3, 4};
int velicina_zad[] = {0, BROJ_A, BROJ_B, BROJ_C};
int pomak_indeksa[] = {0, BROJ_A, BROJ_A+BROJ_B, BROJ_A+BROJ_B+BROJ_C}; //pomak pokazivaca
bool kraj_simulacije = false;


int daj_iduci(){
    int sljedeci=0;
    int izlaz = -1;
        if(red[redosljed] != -1){
            izlaz = zad[trenutni_zad[red[redosljed]]+pomak_indeksa[red[redosljed]]]-1; //OVO JE DOBRO
            //izlaz = (zad[pomak_indeksa[red[redosljed]]])-1;
        // std::cout<<izlaz<<std::endl;
            sljedeci = trenutni_zad[red[redosljed]]+1;
            if(sljedeci >= velicina_zad[red[redosljed]+1]){
                sljedeci = 0;
            }
            trenutni_zad[red[redosljed]] = sljedeci;
        }
    redosljed++;
        if(redosljed >= VELICINA_REDA){
            redosljed=0; //izbjegavanje overflowa
        }
    return izlaz;
}

void simuliraj_dio_obrade(){
            vTaskDelay(10/portTICK_PERIOD_MS);

    }


void init_gpio_in(int port, int i){
    m_pinNumber_in[i] = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port[%d] input!!!", port);
    gpio_reset_pin(m_pinNumber_in[i]);
    /* Set the GPIO as a output */
    gpio_set_direction(m_pinNumber_in[i], GPIO_MODE_INPUT);
    gpio_set_pull_mode(m_pinNumber_in[i],GPIO_PULLUP_ONLY);
}

void init_gpio_out(int port, int i){m_pinNumber_out[i] = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port[%d] output!!!", port);
    gpio_reset_pin(m_pinNumber_out[i]);
    /* Set the GPIO as a output */
    gpio_set_direction(m_pinNumber_out[i], GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(m_pinNumber_out[i], GPIO_FLOATING);}


void inicijalizacija(){
    init_gpio_in(SIMULATOR_IN_GPIO_A,0);
    init_gpio_out(SIMULATOR_OUT_GPIO_A,0);

    init_gpio_in(SIMULATOR_IN_GPIO_B,1);
    init_gpio_out(SIMULATOR_OUT_GPIO_B,1);

    init_gpio_in(SIMULATOR_IN_GPIO_C,1);
    init_gpio_out(SIMULATOR_OUT_GPIO_V,1);

    init_gpio_in(SIMULATOR_IN_GPIO_D,1);
    init_gpio_out(SIMULATOR_OUT_GPIO_D,1);

    for (int i=0;i<BROJ_ULAZA; i++){
        gpio_set_level(m_pinNumber_out[i], LOW); //signal simulatorima da pocnu
    }
    vTaskDelay(20/portTICK_PERIOD_MS); //javi simulatoru i cekaj 15 milisekundi
    for (int i=0;i<BROJ_ULAZA; i++){
        gpio_set_level(m_pinNumber_out[i], HIGH);
    }

}

extern "C" void app_main(void)
{
int i=-1;
    int trenutna_perioda = 0;
    int prekoracena_perioda = (trenutna_perioda);

    bool zadatak_u_obradi = false;
    bool gotova_obrada = true;
    bool moze_dalje = false;
    int o = 0;

    int brojac_praznog_hoda = 0;

    inicijalizacija();

    while(!kraj_simulacije){
     //   std::cout<<"wait: " << wait << std::endl;
        vTaskDelay(100/portTICK_PERIOD_MS);
        if(!gotova_obrada){
            //provjera dali treba prekinuti zadatak
            if(potroseno_perioda[i] >= 1 && ((trenutna_perioda - prekoracena_perioda) < POMAK_PERIODA_PREKORACENJA)){
                moze_dalje = false;  //prekidanje zadatka
                std::cout<< "zadatak " << i+1 << " je prekoracio ogranicenja i treba ga prekinuti" << std::endl;
            }
            else{               

                potroseno_perioda[i] += 1;
                prekoracena_perioda = trenutna_perioda;
                moze_dalje = true;
                std::cout<< "zadatak " << i+1 << " nije prekoracio ogranicenja i moze nastaviti rad" << std::endl;
            }
        }

        //odabir slijedeceg zadatka prema tablici
        if(gotova_obrada || !moze_dalje){
            i = daj_iduci();
            bool simulatorState;

            simulatorState=gpio_get_level(m_pinNumber_in[i]);
            while(simulatorState == HIGH){ //ako ulaz nije spreman trazim sljedeci koji je
                 i = daj_iduci();
                 simulatorState=gpio_get_level(m_pinNumber_in[i]);
                vTaskDelay(15/portTICK_PERIOD_MS);
                if(brojac_praznog_hoda > 1000){
                    kraj_simulacije = true;
                    break;
                }
                brojac_praznog_hoda++;

            }
            
            brojac_praznog_hoda = 0;
            gotova_obrada = false;
            o=0;
            if(i != -1){
              //  std::cout << "Simuliram obradu " << i+1 << std::endl;
                potroseno_perioda[i] = 0;
                int brojac = brojac_simulator[i]; //osiguravanje da se spreme podaci za trenutnu iteraciju, nebino za popunu

            }
        }
            while(i != -1){
                simuliraj_dio_obrade(); //čekajnje
                o+=100;
                if(o >= ulaz[i].C){
                    gotova_obrada = true;
                    //std::cout<<"gotov";
                    break;
                }
                if(o % 100 == 0){
                    //std::cout<<"kratim ";
                    break;
                }
            }


            if(gotova_obrada && i!=-1){
              //  std::cout << "Upr" << u+1 <<": Gotova obrada ulaza-" << i+1 <<std::endl;
                gotovo[i] = true;
                gpio_set_level(m_pinNumber_out[i], LOW);
                vTaskDelay(30/portTICK_PERIOD_MS); //javi simulatoru i cekaj 15 milisekundi
                gpio_set_level(m_pinNumber_out[i], HIGH);

                //gotova_obrada = false;
               // o=0;
                std::cout<<"."<<std::endl;
            }

        trenutna_perioda++;
       
    }
  //  return 0;
}
