# Random Publications & Subscriptions Generator

## Descriere

Acest proiect implementeaza un generator de date pentru sisteme de tip publish–subscribe. Programul genereaza aleator:

- Publicatii – cu structura fixa de campuri
- Subscriptii – cu campuri optionale si frecvente configurabile

Implementarea permite control asupra:
- numarului total de mesaje
- frecventei campurilor in subscriptii
- ponderii operatorilor (ex. "=" pentru `company`)
- paralelizarii generarii pentru optimizarea performantei

---

## Structura datelor

### Publicatie (structura fixa)
{(company,"Google");(value,90.0);(drop,10.0);(variation,0.73);(date,02.02.2022)}

Campuri:
- company – string (ales din set predefinit)
- value – double (interval configurabil)
- drop – double (interval configurabil)
- variation – double (interval configurabil)
- date – string (ales din set predefinit)

---

### Subscriptie (structura variabila)
{(company,=,"Google");(value,>=,90);(variation,<,0.8)}

Caracteristici:
- campurile pot lipsi
- frecventa aparitiei campurilor este configurabila
- pentru campul `company` se impune un procent minim pentru operatorul `=`

---

## Configurare

Parametrii principali:
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
- fiecare thread genereaza un subset independent

---

## Evaluare performanta

Configuratie test:
- Publicatii: 200000
- Subscriptii: 200000

Rezultate:

Threads: 1
Generare: 0.601908 sec
Total:    0.601908 sec

Threads: 4
Generare: 0.25696 sec
Total:    0.25696 sec

Generare + scriere:

Timp generare: 0.218646 sec
Timp scriere:  1.35955 sec
Timp total:    1.57819 sec

---

## Analiza

- Paralelizarea reduce timpul de generare de ~2.3x
- Scrierea in fisier este bottleneck-ul principal

---

## Specificatii sistem

(completeaza tu)

CPU:
RAM:
OS:

---

## Concluzie

Cerintele sunt indeplinite:
- generare configurabila
- distributii controlate
- paralelizare
- evaluare performanta
