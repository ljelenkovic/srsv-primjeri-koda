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

#define BROJ_ITERACIJA 50
int brojac_iteracija = 0;

bool gotovo[BROJ_ULAZA] = {false};
int potroseno_perioda[BROJ_ULAZA] = {0};

int brojac_simulator[BROJ_ULAZA] = {0};
int trajanje_reakcije[BROJ_ULAZA][BROJ_ITERACIJA] = {-1};
int brojac_neobradjenih[BROJ_ULAZA] = {0};
bool promjena[BROJ_ULAZA] = {false};


#define HIGH 1
#define LOW 0

#define IN_GPIO 1
#define OUT_GPIO 0


gpio_num_t m_pinNumber_in;
gpio_num_t m_pinNumber_out;
int trenutak_promjene[BROJ_ULAZA][BROJ_ITERACIJA] = {-1};
int trenutak_reakcije[BROJ_ULAZA][BROJ_ITERACIJA] = {-1};



const char *LogName = "simulator";

std::chrono::steady_clock::time_point begin;

typedef struct input_data {
    int T, t0, C, x;
};

input_data ulaz[] = { //{.T, .t0, .C, .x} x=1 stvarni ulaz, x=0 popuna (LAB2)
//ovaj redak svake sekunde
{ 1000, 0, 70, 1}, {1000, 300, 70, 1}, {3000, 600, 150, 1}, {3000, 600, 150, 1}};

//tablica reaspoređivanja
int red[] = { 0, 1, -1, 0, 1, -1, 0, 1, 2, -1 };  //za svaki prekid svakih 100ms

//int zadA[] = {1, 2, 3};
//int zadB[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
//int zadC[] = {18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37};
int zad[] = {1, 2, 3, 4};
int velicina_zad[] = {0, BROJ_A, BROJ_B, BROJ_C};
int pomak_indeksa[] = {0, BROJ_A, BROJ_A+BROJ_B, BROJ_A+BROJ_B+BROJ_C}; //pomak pokazivaca
bool kraj_simulacije = false;

void ispisi_statistiku(int i){
    int max_reak=-1, avg_reak=0, brojac =0;
    //racunjanje prosjecnog i maksimalnog trajanja reakcije za Ulaz-i
    for(int j=0; j<BROJ_ITERACIJA; j++){
        if(trajanje_reakcije[i][j] > -1){
            if(trajanje_reakcije[i][j] > max_reak){
                max_reak = trajanje_reakcije[i][j];
            }
            avg_reak = avg_reak + trajanje_reakcije[i][j];
            brojac++;
        }
    }
    if(brojac > 0){
        avg_reak = (int)(avg_reak/brojac);
    } else{
        avg_reak = 0;
    }
    std::cout << "Ulaz-" << i+1 << " broj promjena stanja: " << brojac_simulator[i] << std::endl;
    std::cout << "Ulaz-" << i+1 << " prosječno vrijeme reakcije na promjenu stanja: " << avg_reak << " ms" << std::endl;
    std::cout << "Ulaz-" << i+1 << " maksimalno vrijeme reakcije na promjenu stanja: " << max_reak << " ms" << std::endl;
    std::cout << "Ulaz-" << i+1 << " broj neobrađenih događaja: " << brojac_neobradjenih[i] << std::endl;
}



void init_gpio_in(int port){
    m_pinNumber_in = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port[%d] input!!!", port);
    gpio_reset_pin(m_pinNumber_in);
    /* Set the GPIO as a output */
    gpio_set_direction(m_pinNumber_in, GPIO_MODE_INPUT);
    gpio_set_pull_mode(m_pinNumber_in,GPIO_PULLUP_ONLY);
}

void init_gpio_out(int port){m_pinNumber_out = (gpio_num_t)port;
    ESP_LOGI(LogName, "Configure port[%d] output!!!", port);
    gpio_reset_pin(m_pinNumber_out);
    /* Set the GPIO as a output */
    gpio_set_direction(m_pinNumber_out, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(m_pinNumber_out, GPIO_FLOATING);
    }


void inicijalizacija(){
    init_gpio_in(IN_GPIO);
    init_gpio_out(OUT_GPIO);

    std::cout<<"palim cekanje simulatora upravlajca"<<std::endl;
    vTaskDelay(20/portTICK_PERIOD_MS); //cekaj stabilizaciju

    while(gpio_get_level(m_pinNumber_in) == HIGH){
        vTaskDelay(20/portTICK_PERIOD_MS); //cekaj da se simulator javi
        std::cout<<"cekam simulator upravlajca"<<std::endl;
    }
    vTaskDelay(10/portTICK_PERIOD_MS); //cekaj jos malo

}

void simulacija_ulaza(int i){
       if(ulaz[i].x == 0){
        std::cout << "ulaz-" << i+1 << " je ulaz za popunu" << std::endl;
        return;
    }
    else{
        int t= ulaz[i].t0; //trenutak prve pojave
        int d=K*ulaz[i].T+X;  //rok za obradu
        //int d=100; //100 milisekundi
        while(!kraj_simulacije){
            while(std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count() < t){
                //rasterecivanjeprocesora
                vTaskDelay(SIMULATOR_CEKANJE/portTICK_PERIOD_MS);
            }
            promjena[i] = true; //promjena stanja na ulazu
            gpio_set_level(m_pinNumber_out, LOW);
            std::cout << "Ulaz-" << i+1 << " promjena ulaza" << std::endl;
            trenutak_promjene[i][brojac_simulator[i]] = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count(); //biljezenje trenutka promjene

            //cekaj kraj obrade ili istek periode
            while(!promjena[i] || t+d > std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count()){
                vTaskDelay(SIMULATOR_CEKANJE/portTICK_PERIOD_MS);

            }
            promjena[i] = false;
            gpio_set_level(m_pinNumber_out, HIGH);
            if(gpio_get_level(m_pinNumber_in) == LOW){
                gotovo[i] = true;
            }

            if(gotovo[i]){
                trajanje_reakcije[i][brojac_simulator[i]] = trenutak_reakcije[i][brojac_simulator[i]] - trenutak_promjene[i][brojac_simulator[i]];
                std::cout << "Ulaz-" << i+1 << " Gotova obrada: reakcija: " << trajanje_reakcije[i][brojac_simulator[i]] << " ms" << std::endl;
            } else{
                brojac_neobradjenih[i]++;
            }
            gotovo[i] = false;
            t = t+ulaz[i].T;
            brojac_simulator[i]++;
            if(brojac_iteracija == BROJ_ITERACIJA){
                kraj_simulacije = true;
            }
            brojac_iteracija++;
        }
    }
    ispisi_statistiku(i);
}

extern "C" void app_main(void)
{


    inicijalizacija();

    simulacija_ulaza(1);
  //  return 0;
}
