# ESP32-C6 rješenje četvrte laboratorijske vježbe

Ovaj repozitorij sadrži dvije aplikacije za ESP32-C6 koje demonstriraju sustav zahtjev-odgovor koristeći GPIO komunikaciju između dva mikrokontrolera. Jedan ESP32 djeluje kao Generator Zahtjeva, dok drugi djeluje kao Obrađivač Zahtjeva.

## Pregled

### **Generator Zahtjeva (App 1)**
- Šalje signale zahtjeva prema Obrađivaču Zahtjeva putem GPIO pina.
- Čeka signal odgovora od obrađivača putem drugog GPIO pina.
- Prati vrijeme generiranja zahtjeva i primitka odgovora.
- Svakih 15 sekundi izračunava i zapisuje statistiku:
  - Najkraće vrijeme odziva
  - Najdulje vrijeme odziva
  - Prosječno vrijeme odziva
  - Ukupan broj obrađenih zahtjeva

### **Obrađivač Zahtjeva (App 2)**
- Prima signale zahtjeva od Generatora Zahtjeva putem GPIO pina.
- Obrađuje zahtjeve simulirajući vrijeme obrade od 100ms.
- Šalje signal odgovora natrag Generatoru Zahtjeva nakon završetka obrade.
- Podržava konfigurabilno opterećenje dodavanjem dodatnih zadataka koji simuliraju CPU opterećenje.

## Značajke

### Generator Zahtjeva:
- Generira zahtjeve s određenom periodom (1 sekunda).
- Izračunava i zapisuje statistiku o vremenima odziva.

### Obrađivač Zahtjeva:
- Koristi GPIO ulaz za otkrivanje signala zahtjeva i GPIO izlaz za slanje odgovora.
- Obrađuje zahtjeve koristeći RTOS zadatke i ISR (prekidne rutine).
- Omogućuje konfiguraciju razine opterećenja putem makroa `WORKLOAD_INTENSITY`:
  - Vrijednost 1: minimalno opterećenje.
  - Vrijednost 10: maksimalno opterećenje.

## Upute za Korištenje

### Hardverska Postavka
- Spojite GPIO4 (Generator Zahtjeva) na GPIO4 Obrađivač Zahtjeva.
- Spojite GPIO5 (Generator Zahtjeva) na GPIO5 (Obrađivač Zahtjeva).
- Osigurajte zajedničko uzemljenje između oba ESP32 uređaja.

### Kompilacija i Pokretanje
1. Postavite ESP-IDF razvojno okruženje: [ESP-IDF Dokumentacija](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
2. Za Generator Zahtjeva:
   - Otvorite kôd iz `interrupt_generator/main.c`.
   - Kompajlirajte i flashajte kôd na prvi ESP32-C6 uređaj.
3. Za **Obrađivač Zahtjeva**:
   - Otvorite kôd iz `interrupt_generator/interrupt_handler/main.c`.
   - Kompajlirajte i flashajte kôd na drugi ESP32-C6 uređaj.
4. Spojite oba uređaja prema uputama za hardversku postavku.

## Konfiguracija

### Promjena Perioda Zahtjeva (Generator Zahtjeva)
- U funkciji `vTaskDelay` unutar `request_generator_task` možete promijeniti period između zahtjeva:
  ```c
  vTaskDelay(pdMS_TO_TICKS(1000));  // Promijenite 1000 za drugačiju periodu u ms
  ```

### Konfiguracija Opterećenja (Obrađivač Zahtjeva)
- Razinu opterećenja možete postaviti promjenom vrijednosti makroa `WORKLOAD_INTENSITY`:
  ```c
  #define WORKLOAD_INTENSITY 10  // Postavite 1-10 ovisno o željenom opterećenju
  ```

---

## Statistika
- Generator Zahtjeva svakih 15 sekundi ispisuje sljedeće podatke:
  - Najkraće vrijeme odziva.
  - Najdulje vrijeme odziva.
  - Prosječno vrijeme odziva.
  - Ukupan broj obrađenih zahtjeva.
