# 🎢 Progetto Carousel — Computer Grafica (a.a. 2023/24)

## 🖼️ Descrizione Generale

Questo progetto nasce come estensione del codice base fornito dal corso di **Computer Grafica** (Università di Pisa, Prof. G. Ganovelli). L'obiettivo era sviluppare un'applicazione in C++ (ambiente Linux) interattiva che mostrasse e permettesse di manipolare una scena animata contenente un carosello di automobili, su un terreno realistico, con vari elementi grafici e illuminazione.

Il codice di partenza era disponibile nella directory `src/common/carousel` del repository ufficiale del corso. A partire da questa base, sono state implementate tutte le **funzionalità richieste** e alcune migliorie personali.

---

## 🔧 Funzionalità Implementate

### Visualizzazione della scena

- Navigazione del punto di vista: è possibile osservare la scena da ogni angolazione.
- Il terreno viene generato da un campo di altezze e visualizzato con texture tileable.

### Texturing

- Il terreno ha una texture tileable ottenuta da `terrain_256.png`, mappata automaticamente sui vertici.
- La pista è anch'essa texturizzata: le coordinate sono calcolate in base alla distanza progressiva dal punto di partenza.

### Mesh e oggetti

- Aggiunti e visualizzati correttamente:
  - Lampioni
  - Alberi
  - Auto animate
  - Cameramen

### Illuminazione

- **Luce solare direzionale** integrata nella scena.
- **Illuminazione locale da lampioni** realistica e coerente.
- **Proiezione dei fanali delle auto** implementata tramite texturing proiettivo.

### Ombre

- Ombre generate da tutte le sorgenti luminose: sole, lampioni e fanali delle auto.

---

## Punti aggiuntivi

- Lavoro effettuato in modo modulare: ogni elemento grafico o di illuminazione è stato testato separatamente prima dell'integrazione.
- Cura particolare al bilanciamento tra qualità visiva e prestazioni.
- Scene navigabili senza lag o glitch, con buona gestione delle texture e della memoria.

---

## 🔍 Approccio seguito

- Dopo una fase iniziale di studio del codice fornito, l'attenzione si è focalizzata sulla generazione del terreno e sulla gestione delle texture.
- In seguito si è lavorato sull'aggiunta degli oggetti 3D (modelli già inclusi o reperiti da fonti gratuite), impostando materiali e luci.
- Infine, è stato integrato il sistema di ombreggiatura e illuminazione multipla, sfruttando ciò che era stato visto a lezione (e approfondendo dove necessario).

---

## 🎞️ Prima e Dopo

### Prima — Griglia e struttura base della scena
<img width="544" height="564" alt="Pre-rendering" src="https://github.com/user-attachments/assets/6a23e7f9-a502-4545-884b-46db9f178455" />

### Dopo — Risultato finale con texture, ombre e illuminazione
<img width="544" height="564" alt="Post-rendering" src="https://github.com/user-attachments/assets/0cf3aab4-2aea-4085-88ba-e10c6d769f04" />

---

## 🚀 Come eseguire il progetto

Assicurarsi di avere un ambiente compatibile con OpenGL + GLFW e un compilatore C++ moderno. Poi:

```bash
make
./main
```

---

## 📁 Struttura del progetto

```
carousel_project/
├── main.cpp
├── makefile
├── terrain_256.png
├── small_test.svg
├── src/
│   └── common/
│       └── carousel/
│           ├── ... (codice base fornito)
└── ...
```

---

## 🙌 Ringraziamenti

Grazie al docente Prof. Ganovelli per la base solida fornita, che ha reso possibile concentrarsi sugli aspetti più creativi e pratici del rendering interattivo in 3D. È stata una buona palestra per consolidare OpenGL, illuminazione e manipolazione di mesh dinamiche.

