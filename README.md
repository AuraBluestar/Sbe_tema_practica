# Content-Based Publish/Subscribe System

## Descriere

Acest proiect implementeaza un sistem publish/subscribe content-based, construit peste generatorul de publicatii si subscriptii realizat anterior.

Sistemul contine:

- Publisheri - noduri care genereaza publicatii random si le trimit catre reteaua de brokeri
- Brokeri - noduri intermediare care stocheaza subscriptii, fac matching content-based si ruteaza publicatii/notificari
- Subscriberi - noduri client care genereaza subscriptii random si primesc notificari pentru publicatiile care se potrivesc

Implementarea urmareste cerintele temei:

- 1-2 publisheri care emit fluxuri de publicatii
- overlay de 2-3 brokeri
- 2-3 subscriberi care inregistreaza subscriptii
- routing avansat pentru subscriptii si publicatii
- evaluare pentru 10000 subscriptii si feed continuu de 3 minute

---

## Structura datelor

### Publicatie

Publicatia are structura fixa:

```text
{(id,1000000001);(company,"Google");(value,90.0);(drop,10.0);(variation,0.73);(date,02.02.2022)}
```

Campuri:

- `id` - identificator unic folosit pentru deduplicare
- `company` - string, ales dintr-un set predefinit
- `value` - double
- `drop` - double
- `variation` - double
- `date` - string, ales dintr-un set predefinit

ID-ul publicatiei este important deoarece aceeasi publicatie poate trece prin mai multi brokeri. Brokerii si subscriberii folosesc ID-ul pentru a evita livrarile duplicate.

---

### Subscriptie

Subscriptia are structura variabila:

```text
{(company,=,"Google");(value,>=,90);(variation,<,0.8)}
```

Caracteristici:

- campurile pot lipsi
- frecventa aparitiei campurilor este configurabila
- pentru campul `company` se controleaza procentul operatorului `=`
- operatorii numerici folositi sunt `<`, `<=`, `>`, `>=`
- operatorii pe string sunt `=` si `!=`

---

## Arhitectura sistemului

### Publisher

Fisier principal:

```text
tema2/publisher.cpp
```

Publisherul:

- primeste ca parametri ID-ul, numarul de publicatii, numarul de brokeri si delay-ul intre publicatii
- alege aleator un broker de intrare
- genereaza publicatii random folosind generatorul din tema anterioara
- trimite publicatiile catre brokerul ales

Exemplu:

```bash
./publisher 1 10 3 100
```

Parametri:

- `1` - ID publisher
- `10` - numar publicatii
- `3` - numar brokeri
- `100` - delay in ms intre publicatii

---

### Subscriber

Fisier principal:

```text
tema2/subscriber.cpp
```

Subscriberul:

- genereaza subscriptii random
- se conecteaza la toti brokerii pentru a putea primi notificari pe brokerul de intrare folosit
- pentru fiecare subscriptie alege aleator un broker de intrare
- trimite subscriptia catre brokerul ales
- primeste notificari si deduplicateaza dupa ID-ul publicatiei

Exemplu:

```bash
./subscriber 1 10 3 100
```

Parametri:

- `1` - ID subscriber
- `10` - numar subscriptii
- `3` - numar brokeri
- `100` - procent de subscriptii cu `company = ...`

---

### Broker

Fisier principal:

```text
tema2/broker.cpp
```

Brokerul:

- asculta conexiuni de la publisheri, subscriberi si alti brokeri
- se conecteaza la ceilalti brokeri din overlay
- stocheaza subscriptii
- face matching content-based intre publicatii si subscriptii
- forwardeaza publicatii catre brokerii responsabili pentru compania publicatiei
- ruteaza subscriptii catre brokerii responsabili
- intoarce notificari catre brokerul de intrare al subscriberului
- elimina duplicatele folosind ID-ul publicatiei

Exemplu:

```bash
./broker 1 3
./broker 2 3
./broker 3 3
```

Parametri:

- primul argument - ID broker
- al doilea argument - numarul total de brokeri

---

## Routing

### Responsabilitate pe companii

Fiecare companie este asociata determinist cu 1-2 brokeri. Exemplu pentru 3 brokeri:

| Companie | Brokeri responsabili |
|---|---|
| Google | B1, B2 |
| Amazon | B2, B3 |
| Microsoft | B1, B3 |
| Apple | B1, B2 |
| Meta | B2, B3 |
| Netflix | B1, B3 |

Aceasta impartire permite ca publicatiile sa nu fie procesate de un singur broker central, ci sa treaca prin mai multi brokeri din overlay.

---

### Routing pentru subscriptii

Subscriberul nu decide brokerul final care stocheaza subscriptia. El alege doar un broker de intrare random.

Brokerul de intrare aplica politica de routing:

- pentru `company = X`, subscriptia este trimisa balansat catre unul dintre brokerii responsabili pentru compania `X`
- pentru subscriptii complexe sau cu `company != X`, subscriptia este stocata local si replicata in overlay
- brokerul care stocheaza subscriptia retine si brokerul de origine, pentru a putea intoarce notificarea catre subscriber

Astfel, subscriptiile aceluiasi subscriber sunt distribuite pe mai multi brokeri, iar decizia de routing apartine overlay-ului, nu clientului.

---

### Routing pentru publicatii

Publisherul trimite publicatia catre un broker random.

Brokerul care primeste publicatia:

1. verifica daca publicatia a mai fost procesata
2. determina brokerii responsabili pentru `company`
3. proceseaza local daca este responsabil
4. forwardeaza publicatia catre ceilalti brokeri responsabili
5. brokerii responsabili fac matching local si trimit notificari catre brokerul de origine al subscriberului

Pentru a evita buclele si duplicatele:

- fiecare broker tine `processedPublications`
- fiecare broker tine `deliveredToSubscriber`
- fiecare subscriber tine local publicatiile deja primite

---

## Tipuri de mesaje

Mesajele sunt definite in:

```text
tema2/common/message.h
```

Tipuri folosite:

- `SUBSCRIPTION` - subscriber catre broker
- `PUBLICATION` - publisher catre broker sau broker catre broker
- `NOTIFICATION` - broker catre subscriber
- `BROKER_HELLO` - identificare conexiuni intre brokeri
- `BROKER_SUBSCRIPTION` - routing intern pentru subscriptii
- `ROUTED_NOTIFICATION` - notificare intoarsa catre brokerul de intrare

---

## Mod rulare

### Compilare

Din directorul `tema2`:

```bash
bash build.sh
```

Sau din radacina proiectului:

```bash
bash build.sh
```

---

### Rulare manuala

Se pornesc mai intai brokerii:

```bash
./broker 1 3
./broker 2 3
./broker 3 3
```

Apoi subscriberii:

```bash
./subscriber 1 10 3 100
./subscriber 2 10 3 100
```

Apoi publisherul:

```bash
./publisher 1 10 3 100
```

---

### Rulare automata

Pentru a evita pornirea manuala in mai multe terminale, exista scriptul:

```text
tema2/run_system.sh
```

Exemplu:

```bash
bash run_system.sh 3 2 1 10 10 100 100
```

Parametri:

| Parametru | Semnificatie |
|---|---|
| `3` | numar brokeri |
| `2` | numar subscriberi |
| `1` | numar publisheri |
| `10` | subscriptii per subscriber |
| `10` | publicatii per publisher |
| `100` | procent `company = ...` |
| `100` | delay publisher in ms |

Scriptul:

- compileaza executabilele
- opreste procese vechi
- porneste brokerii
- porneste subscriberii
- porneste publisherii
- salveaza loguri in `tema2/logs/`
- afiseaza un sumar cu notificari, forward-uri, duplicate si rutarea subscriptiilor

---

## Evaluare

Evaluarea automata este facuta cu:

```text
tema2/run_evaluation.sh
```

Comanda folosita pentru testul final:

```bash
bash run_evaluation.sh 3 3 1 180 10000 10 8
```

Parametri:

| Parametru | Valoare | Semnificatie |
|---|---:|---|
| Brokeri | 3 | overlay cu 3 noduri broker |
| Subscriberi | 3 | noduri subscriber simulate |
| Publisheri | 1 | nod publisher activ |
| Durata | 180 sec | feed continuu de 3 minute |
| Subscriptii | 10000 | numar total de subscriptii |
| Delay publisher | 10 ms | interval intre doua publicatii |
| Stabilizare | 8 sec | timp pentru inregistrarea subscriptiilor inainte de feed |

Scriptul ruleaza doua scenarii:

- `company = ...` in 100% dintre subscriptii
- `company = ...` in aproximativ 25% dintre subscriptii

---

## Rezultate evaluare

Rezultatele obtinute pentru rularea de 3 minute:

| Scenariu | Publicatii trimise | Publicatii unice livrate | Notificari livrate | Latenta medie | Matching rate |
|---|---:|---:|---:|---:|---:|
| Company EQ 100% | 18000 | 18000 | 50542 | 1003.44 us  | 0.028073% |
| Company EQ 25% | 18000 | 18000 | 54000 | 1178.42 us | 0.029994% |

Matching rate este calculata ca:

```text
notificari_livrate / (publicatii_trimise * subscriptii_generate) * 100
```

---

## Comparatie 100% EQ vs 25% EQ

In scenariul 100% EQ, fiecare subscriptie contine o conditie stricta de forma:

```text
company = X
```

O publicatie se potriveste doar cu subscriptiile care cer exact compania publicatiei.

In scenariul 25% EQ, doar un sfert dintre subscriptii folosesc operatorul `=`, iar restul folosesc in principal conditii de tip:

```text
company != X
```

Aceste conditii sunt mai largi, deoarece o subscriptie `company != Google` se potriveste cu publicatii pentru Amazon, Microsoft, Apple etc. Din acest motiv, numarul total de notificari livrate si matching rate-ul sunt mai mari in cazul 25% EQ.

Comparatie observata:

- 100% EQ: 50542 notificari livrate
- 25% EQ: 54000 notificari livrate

Diferenta confirma ca scaderea frecventei operatorului de egalitate creste numarul de potriviri.

---

## Analiza

- Toate cele 18000 publicatii au fost livrate catre cel putin un subscriber in ambele scenarii.
- Publicatiile trec prin mai multi brokeri, lucru vizibil in logurile `Forwarded PUBLICATION`.
- Subscriptiile sunt rutate prin overlay, lucru vizibil in logurile `Routed SUBSCRIPTION` si `Stored routed SUBSCRIPTION`.
- Deduplicarea functioneaza pe baza ID-ului de publicatie.
- Latenta medie ramane in ordinul microsecundelor/milisecundelor mici pentru rularea locala.
- Scenariul 25% EQ produce mai multe match-uri deoarece operatorul `!=` este mai permisiv decat `=`.

---

## Observatii despre implementare

Implementarea foloseste socket-uri TCP locale si thread-uri pentru conexiuni. Fiecare broker accepta conexiuni de la publisheri, subscriberi si peer brokeri. Pentru testare si evaluare, scripturile automatizeaza pornirea proceselor si colectarea logurilor.

Logurile generate nu sunt versionate, fiind ignorate prin `.gitignore`.
