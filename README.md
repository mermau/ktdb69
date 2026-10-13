# KTKTDB-69 (LULL Audio) — emulazione del filtro passa-alto passivo Altec 9069B (VST3 / AU / Standalone)

Plugin in C++17 con JUCE 8. Riproduce il "Big Knob" di King Tubby: il filtro passa-alto passivo
Altec 9069B (rete LC da 600 Ω, 18 dB/ottava, 10 frequenze da 70 Hz a 7.5 kHz più OFF), con i tre
modi del KTBK-1B di Audio Merge, che ha fornito le curve di riferimento.

![Interfaccia](docs/screenshot.png)

## Grafica

Pannello verde oliva, font TeX Gyre Adventor (stile Avant Garde, incorporato nel plugin) e logo
LULL Audio con tre curve passa-alto: tre scatti della manopola grande. Lo stesso logo è l'icona
dell'app Standalone. "TYPE 9069 DUB FILTER" è spaziato per occupare esattamente la larghezza di
"KTDB-69", e "AUDIO" quella di "LULL": la spaziatura si calcola sui contorni reali delle lettere
(`ui::drawJustified` in `Source/Theme.h`).

Il font è distribuito con la GUST Font License (`resources/fonts/GUST-FONT-LICENSE.txt`), che ne
consente l'incorporamento. L'icona si rigenera con `gui_snapshot resources/icon.png`.

## Comandi

Gli stessi del pannello hardware:

| Comando | Posizioni | CC predefinito |
|---|---|---|
| **Manopola grande** (LOW CUT FREQUENCY) | OFF, 70, 100, 150, 250, 500, 1KC, 2KC, 3KC, 5KC, 7.5KC | 20 |
| **BELL** | 6 livelli, MIN … MAX (DF0 … DF5) | 21 |
| **MODE** | OUT (true bypass), BELL, 600, NOTCH | 22 |
| Pulsante **IN/OUT** (commuta tra OUT e l'ultimo modo usato) | — | 64 (pedale sustain) |
| INPUT / OUTPUT (barra superiore, ±24 dB) | — | — |
| CLICK SELETTORE (barra superiore) | acceso / spento | — |

- **600** — la risposta originale del 9069B: sorgente e carico da 600 Ω, Butterworth del 3° ordine
  con 6 dB di perdita di inserzione (come l'originale: alza OUTPUT se serve).
- **BELL** — il suono di King Tubby: la rete pilotata dall'uscita a bassa impedenza del banco e
  caricata da un ingresso "a ponte" non è terminata e risuona. Il picco cade a circa 1.37 volte la
  frequenza indicata e arriva a +10 dB (70 Hz) … +13 dB (7.5 kHz) con BELL al massimo; MIN non ha picco.
  Nessuna perdita di inserzione.
- **NOTCH** — gli stessi componenti ricollegati come ramo LC serie verso massa: reiezione centrata
  sulla frequenza della manopola, da −22 dB (70 Hz) a −42 dB (7.5 kHz) con BELL al massimo; il livello
  BELL riduce la profondità. (Questo modo è un'aggiunta del KTBK-1B, non esiste sul 9069B.)
- **OUT** — true bypass, latenza compensata.

**Click del selettore** (acceso di serie): come un vero commutatore rotativo, a ogni scatto il
contatto si interrompe per 1.5 ms e i nuovi componenti entrano nel circuito scarichi: nascono click e
transitori reali, che in BELL suonano come il tipico "colpo" risonante delle sweep dub. Spento, il
passaggio tra le posizioni è una dissolvenza di 25 ms.

Uso con il mouse: trascina in verticale (o in orizzontale), usa la rotella, oppure clicca
direttamente su una scritta della scala per saltare a quella posizione.

## Tastiera MIDI (Arturia KeyLab)

**MIDI Learn**: clic destro su un comando → *MIDI Learn* → muovi un encoder, un fader o un pulsante
della KeyLab. La mappatura è salvata nel progetto. Il pulsante IN/OUT si assegna dal menu del clic
destro sulla manopola MODE. I comandi a scatti hanno un'isteresi: un fader fermo a cavallo tra due
posizioni non fa "tremolare" il selettore.

- I pulsanti (IN/OUT, CLICK) commutano alla pressione: in Arturia MIDI Control Center imposta il
  pulsante in modalità *Gate/Momentary*.
- Gli encoder della KeyLab vanno lasciati in modalità *Absolute* (0–127).
- **Ableton Live**: metti il plugin sulla traccia; crea una traccia MIDI con ingresso KeyLab e in
  *MIDI To* scegli la traccia del filtro → "KTKTDB-69". In alternativa usa la mappatura MIDI
  di Live (Cmd+M) sui parametri del plugin.
- **Logic Pro**: l'AU è un normale effetto (Audio FX) e non riceve CC direttamente: usa
  l'assegnazione controller di Logic (Cmd+K, muovi il comando nel plugin, poi quello sulla KeyLab).
  MIDI Learn interno e CC predefiniti valgono per VST3 e Standalone.

## Scaricare la versione per Mac (Apple Silicon e Intel)

Ogni modifica su `main` viene compilata da GitHub Actions su macOS (universal binary: M1–M4 e Intel),
con i test del modello e la validazione `auval` dell'Audio Unit.

1. Nel repository apri **Actions → Build macOS**, scegli l'ultima esecuzione riuscita e scarica
   l'artefatto **KTDB69-macOS**. Per una versione "ufficiale" crea un tag `v1.0.0`: la build compare
   anche in **Releases**, scaricabile senza login.
2. Estrai lo zip (contiene a sua volta `KTDB69-macOS.zip`, estrai anche quello).
3. Il plugin non è firmato con un certificato Apple Developer: apri il Terminale e trascina dentro la
   cartella `KTKTDB-69` dopo questo comando:
   ```
   xattr -dr com.apple.quarantine 
   ```
   poi fai doppio clic su **Installa KTKTDB-69.command** (copia VST3 e AU nelle cartelle dei plugin
   dell'utente e l'app Standalone in Applicazioni).
4. Riavvia la DAW (in Logic: se non compare, *Plug-in Manager → Reimposta e riscansiona*).

## Il modello

![Curve calcolate dal modello](docs/curve.png)

Rete a T "costante-K" da 600 Ω (la stessa famiglia di filtri Langevin/Altec dell'epoca):

```
Vin ─ Rs ─ A ──||── B ──||── C ─┬─ uscita       C1 = C2 = 1 / (2π·fc·600)
               C1    │    C2    RL              L = 1.05 · 600 / (4π·fc)
                     L (+r)     │
                     ⏚          ⏚
```

| Modo | Rs | RL |
|---|---|---|
| 600 | 600 Ω | 600 Ω |
| BELL | 40 Ω | 480 / 730 / 1060 / 1460 / 2060 / 3400 Ω (MIN … MAX) |
| NOTCH | 600 Ω | 20 kΩ, ramo (C1‖C2) + L con 150 / 100 / 47 / 22 / 10 / 0 Ω in serie |

- Con 600 Ω da entrambi i lati la rete è esattamente una Butterworth del 3° ordine (g = 1, 2, 1):
  −9 dB alla frequenza indicata, −6 dB in banda passante, 18 dB/ottava.
- Gli induttori sono le prese di un toroide: resistenza dell'avvolgimento r = 51 Ω·√(70/fc). Questo
  solo dato, oltre ai valori nominali, fa coincidere le profondità del NOTCH (−22 / −33 / −42 dB a
  70 Hz / 1 kHz / 7.5 kHz) e i picchi BELL con le curve misurate.
- Il fattore 1.05 sulle induttanze (tolleranza dei componenti) porta i picchi BELL a 1.37·fc e i notch
  a 0.98·fc, come nelle misure.
- Il nucleo satura dolcemente (i = φ/L·(1+(φ/φs)²)): a livello di linea è trasparente (0.005 % a
  +4 dBu in 600), si sente spingendo INPUT sui passi bassi, soprattutto in BELL. 0 dBFS = +18 dBu.
- Risoluzione: analisi nodale a 3 nodi con la regola dei trapezi; la non linearità dell'induttore è
  risolta con Newton su una sola variabile (equivalente di Thevenin). La rete gira sempre a
  176–192 kHz (oversampling 4× a 44.1/48 kHz, 2× a 88.2/96 kHz, latenza 6 o 4 campioni): così anche il
  picco BELL a 10 kHz cade dove deve. Circa 1–2 % di un core per canale.

Tutti i valori sono in `struct Design` dentro `Source/AltecNetwork.h`.

**Limiti noti**: lo schema interno del 9069B non è pubblicato; la topologia è quella standard dei
filtri a costante-K da 600 Ω dell'epoca, tarata sulle curve del KTBK-1B (che dichiara di essere
validato sull'originale). La saturazione del nucleo è un'ipotesi ragionevole, non una misura. Il modo
NOTCH riproduce il comportamento descritto da Audio Merge, non un circuito documentato.

## Compilazione

Serve CMake ≥ 3.22. JUCE 8.0.4 viene scaricato automaticamente (oppure `-DJUCE_DIR=/percorso/JUCE`).

**macOS (VST3 + AU, universal binary)** — con Xcode installato:
```
cmake -B build -G Xcode
cmake --build build --config Release
```
**Windows (VST3)** — con Visual Studio 2022:
```
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Test: `network_test` (simulazione contro la risposta teorica, picchi e notch, distorsione, click, CPU).
Con `-DDB69_BUILD_SNAPSHOT=ON` si compilano anche `processor_test` (CC MIDI, livelli, bypass, stato)
e `gui_snapshot` (salva l'interfaccia in PNG).
