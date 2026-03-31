# Random Publications & Subscriptions Generator

## Descriere

Acest proiect implementeaza un generator de date pentru sisteme de tip publish–subscribe. Programul genereaza:

- Publicatii – cu structura fixa de campuri
- Subscriptii – cu campuri optionale si frecvente configurabile

Implementarea permite control asupra:
- numarului total de pubs/subs
- frecventei campurilor in subscriptii
- ponderii operatorilor (ex. "=" pentru `company`)
- paralelizarii generarii pentru optimizarea performantei

---

## Structura datelor

### Publicatie (structura fixa)
{(company,"Google");(value,90.0);(drop,10.0);(variation,0.73);(date,02.02.2022)}

Campuri:
- company – string 
- value – double 
- drop – double 
- variation – double 
- date – string 

---

### Subscriptie (structura variabila)
{(company,=,"Google");(value,>=,90);(variation,<,0.8)}

Caracteristici:
- campurile pot lipsi
- frecventa aparitiei campurilor este configurabila
- pentru campul `company` se impune un procent minim pentru operatorul `=`

---

## Configurare

Parametri principali:
- numPublications – numar publicatii
- numSubscriptions – numar subscriptii
- frecvente campuri:
  - companyFreqPct
  - valueFreqPct
  - dropFreqPct
  - variationFreqPct
  - dateFreqPct
- companyEqMinPct – procent minim pentru operator "="

---

## Paralelizare

Tip paralelizare:
- Thread-based (multithreading)

Implementare:
- generarea este impartita pe thread-uri
- fiecare thread genereaza un subset independent - nu este necesara sincronizare

---

## Mod rulare
Comanda compilare: g++ -Wall -Wextra -g3 src/main.cpp src/publication_generator.cpp src/utils.cpp src/writer.cpp src/subscription_generator.cpp -o main.exe

Exemplu comanda rulare: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 --nrPubs 500000 --nrSubs 500000 --eqCompany 40

Se pot seta, de asemenea, --variationFreq si --dateFreq

Valori default pentru rulare cu "./main": numarul de subs/pubs este de 20000 si doar campul company are frecventa de 100%, impreuna cu frecventa de 70% pentru operatorul =.

---

## Evaluare performanta
Testele au fost facute pentru 1 thread, respectiv 4 threaduri.

### Configuratie hardware
- **Procesor:** Intel(R) Core(TM) i7-8700 CPU
- **RAM:** 16 GB
- **OS:** Windows 11 

### Parametri
| Parametru | Valoare |
|---|---|
| Frecvență company | 90% |
| Frecvență value | 50% |
| Frecvență drop | 60% |
| Procent EQ company* | 70% | 

Procentul EQ company este cel default (70%) din cod, asadar nu e inclus in comanda de rulare.
### Rezultate test 1 - numar pubs/subs 20000

| Scenariul | Nr. threaduri | Timp generare | Timp scriere | Timp total |
|---|---|---|---|---|
| Fără scriere | 1 | 0.0300 sec | - | 0.0300 sec |
| Fără scriere | 4 | 0.0195 sec | - | 0.0195 sec |
| Cu scriere | 1 | 0.0282 sec | 0.0983 sec | 0.1265 sec |
| Cu scriere | 4 | 0.0190 sec | 0.0977 sec | 0.1167 sec |


Comanda rulare test 1: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 


### Rezultate test 2 - numar pubs/ subs 500000
| Scenariul | Nr. threaduri | Timp generare | Timp scriere | Timp total |
|---|---|---|---|---|
| Fără scriere | 1 | 0.6977 sec | - | 0.6977 sec |
| Fără scriere | 4 | 0.4061 sec | - | 0.4061 sec |
| Cu scriere | 1 | 0.7017 sec | 2.4951 sec | 3.1968 sec |
| Cu scriere | 4 | 0.4144 sec | 2.4623 sec | 2.8767 sec |


Comanda rulare test 2: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 --nrPubs 500000 --nrSubs 500000                               


---

## Analiza

- Speedup la generare (1→4 threaduri): 0.0300 / 0.0195
- Scrierea in fisier domina timpul total 
- La volume mai mari (ex: 500.000 mesaje) diferenta dintre 1 și 4 threaduri devine mai pronuntată

