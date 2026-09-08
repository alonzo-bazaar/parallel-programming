# Presentazione l-systems
Presentazione l-systems

# Contenuti di questa repo
Ho scaricato più o meno la qualsiasi qua dentro, è tipo un docker dei poveri ma per compilare un pdf, la cartella contiene
- File della presentazione (`presentation.typ`).
- Script `run` che fa il setup di tutto (se serve) poi runna typst.
- Cartella asset con
  - Il template usato per fare la presentazione (`assets > templates > typslides`)
  - Tutti i font utilizzati per la presentazione (`assets > fonts`), in particolare
    - fira sans (quello default del template `typslides`)
    - jetbrains mono (per stile)
    - comic sans (per ridere).
  - Il file eseguibile per runnare `typst` (`assets > tools > typst` (per linux), `assets > tools > typst.exe` (per windows)), gli eseguibili di typst dal github loro sono tutti compilati staticamente, quindi l'eseguibile da solo basta e avanza e non serve manco installarlo con robe strane in cartelle specifiche.
  - Il file eseguibile dell'interpreter utilizzato dallo script che fa il setup ed esegue typst.
- License di qualsiasi cosa.

# Per editare il documento
Penso vscode sappia gestire i file `.typ` abbastanza tranquillamente,
se non out of the box di sicuro c'è un plugin per typst che puoi scaricarti per farlo funzionare.

# Come compilare il documento
Per compilare sto documento se usa [typst](https://typst.app/), non si fa niente di troppo strano col documento, probabilmente puoi anche prendere il file `typ` e schiaffarlo sulla web app di typst (che è tipo overleaf).  
Per comodita degli autori (abe, di un [singolo autore](https://xkcd.com/1782/)), qui si è messo che abbiamo uno script `run` che si scarica tutto e compila il documento e amen.

## Da webapp
Non so usare programmi web, vedi te se vuoi fare dal sito di typst.

## Da script `run`
Lo script `run` è diviso in due parti

- Una prima scarica tutti gli asset che possono servire, purtroppo per come l'ho scritta questa parte dello script per ora funziona solo da posix (linux o mac), potrebbe funzionare (in parte) da windows ma servirebbe scaricarsi [la versione windows delle coreutil posix](https://github.com/microsoft/coreutils), purtroppo farlo in quel modo non l'ho testata al momento.  
  Ai fini di evitare troppe beghe ho lasciato nella repo tutta la roba che si è scaricato lo script girandolo da me (linux), così quando lo rirunni quella parte viene saltata e l'interpreter non si ammazza in medias res per "manca il comando `ls`".

- Una seconda parta lancia l'eseguibile `typst` (vendorizzato all'interno della repo insieme a tutto il resto, faceva più comodo) e gli passa il file della presentazione, questa parte ti produce in output il pdf abbastanza in fretta .

### per runnare lo script
da posix (linux o mac), puoi fare:
```sh
./assets/tools/shsl ./run
```
o anche semplicemente
```sh
./run
```  
  
da windows, puoi fare:
```sh
.\assets\tools\shsl.exe .\run
```
o anche solo 
```sh
run
```

In caso girare `run` e basta non funziona (non ho una macchina windows per testarlo) ho aggiunto per sicurezza un file `.bat` che esegue `.\assets\tools\shsl.exe .\run`, il comando per runnare il file `.bat` è, banalmente
```sh
run.bat
```
> **NOTA**: i comandi in questione vanno eseguiti col terminale nella cartella dove sta lo script, altrimenti la shell non trova il file `run`.
