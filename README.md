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
- company – string, dintr-un set predefinit (models.h)
- value – double 
- drop – double 
- variation – double 
- date – string, dintr-un set predefinit (models.h)

---

### Subscriptie (structura variabila)
{(company,=,"Google");(value,>=,90);(variation,<,0.8)}

Caracteristici:
- campurile pot lipsi
- frecventa aparitiei campurilor este configurabila
- pentru campul `company` se impune un procent minim pentru operatorul `=`


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


### Parametri rulare
- --nrPubs - numar publicații generate
- --nrSubs - numar subscriptii generate
- --companyFreq - % subscriptii care contin company
- --valueFreq - % subscriptii care contin value
- --dropFreq - % subscriptii care contin drop
- --variationFreq - % subscriptii care contin variation
- --dateFreq - % subscriptii care contin date
- --companyEq - % minim operator = pentru campul company
- --pubValueMin, --pubValueMax - interval valori value in publicatii
- --subValueMin, --subValueMax - interval valori value in subscriptii
- Idem pentru variation si drop referitor la intervalul de valori


Exemple comanda rulare: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 --nrPubs 500000 --nrSubs 500000 --eqCompany 40
./main --nrPubs 500000 --nrSubs 500000 --companyFreq 90 --valueFreq 70 --dropFreq 50 --variationFreq 60 --dateFreq 40 --companyEq 70 --pubValueMin 0 --pubValueMax 100 --subValueMin 10 --subValueMax 90


Valori default pentru rulare cu "./main" se afla in models.h: numarul de subs/pubs este de 20000 si doar campul company are frecventa de 100%, impreuna cu frecventa de 70% pentru operatorul =.
Pentru variation, intervalul este (-1,1), si pentru value si drop (0,100). Companiile si datele sunt alese random dintr-un set predefinit.

---

## Evaluare performanta
Testele au fost facute pentru 1 thread, respectiv 4 threaduri.

### Configuratie hardware
- **Procesor:** Intel(R) Core(TM) i7-8700 CPU (6 nuclee fizice, 12 procesoare logice)
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
### Rezultate test 1 - numar pubs/subs 20000 (numar default)

| Scenariul | Nr. threaduri | Timp generare | Timp scriere | Timp total |
|---|---|---|---|---|
| Fără scriere | 1 | 0.0341 sec | - | 0.0341 sec |
| Fără scriere | 4 | 0.0207 sec | - | 0.0207 sec |
| Cu scriere | 1 | 0.0347 sec | 0.0998 sec | 0.1346 sec |
| Cu scriere | 4 | 0.0222 sec | 0.0967 sec | 0.1189 sec |


Comanda rulare test 1: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 


### Rezultate test 2 - numar pubs/ subs 500000
| Scenariul | Nr. threaduri | Timp generare | Timp scriere | Timp total |
|---|---|---|---|---|
| Fără scriere | 1 | 0.8540 sec | - | 0.8540 sec |
| Fără scriere | 4 | 0.4340 sec | - | 0.4340 sec |
| Cu scriere | 1 | 0.8523 sec | 2.4409 sec | 3.2932 sec |
| Cu scriere | 4 | 0.4469 sec | 2.3905 sec | 2.8374 sec |


Comanda rulare test 2: ./main --companyFreq 90 --valueFreq 50 --dropFreq 60 --nrPubs 500000 --nrSubs 500000                               


---

## Analiza

- Speedup la generare (1 sau 4 threaduri): 0.8540 fata de 0.4340 pentru 500000 sub/pub
- Scrierea in fisier domina timpul total 
- La volume mai mari (ex: 500.000 mesaje) diferenta dintre 1 si 4 threaduri devine mai pronuntata

** Rezultatele din fisierele de output sunt pentru comanda: ./main --nrPubs 500000 --nrSubs 500000 --companyFreq 90 --valueFreq 70 --dropFreq 50 --variationFreq 60 --dateFreq 40 --companyEq 70 --pubValueMin 0 --pubValueMax 100 --subValueMin 10 --subValueMax 90    