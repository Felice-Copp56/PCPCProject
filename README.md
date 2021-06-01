# Game of Life

## PCPC Project 
Felice Coppola : *0522501020*

Project : **Conway's Game of Life**

AWS EC2 instance type: m4.large 

# Conway's Game of Life

![image](https://upload.wikimedia.org/wikipedia/commons/e/e5/Gospers_glider_gun.gif)

Il gioco della vita (Game of Life in inglese) è un automa cellulare sviluppato dal matematico inglese John Conway. Il suo scopo è quello di mostrare come comportamenti simili alla vita possano emergere da regole semplici e interazioni a molti corpi. Esso è in realtà un gioco senza giocaotir poichè la sua evoluzione è determinata dal suo stato iniziale, senza necessità di alcun input da parte di giocatori umani. Si svolge su una griglia di caselle quadrate (celle) che si estende all'infinito in tutte le direzioni, tale griglia viene definita <code>mondo</code>. Ogni cella presenta al più 8 vicini, e gli stati di tutte le celle in un dato istante sono usati per calcolare lo stato delle celle all'istante successivo, quindi tutte le celle vengono aggiornate simultaneamente nel passaggio da un istante a quello successivo, detto <code>generazione</code>
Le transizioni dipendono dallo stato delle celle vicine in quella generazione:

+ Qualsiasi cella viva con meno di due celle vive adiacenti muore, per effetto d'isolamento
+ Qualsiasi cella viva con due o tre celle vive adiacenti sopravvive alla generazione successiva
+ Qualsiasi cella viva con più di tre celle vive adiacenti muore, per effetto di sovrappopolazione
+ Qualsiasi cella morta con esattamente tre celle vive adiacenti diventa una cella viva, per effetto di riproduzione


# Descrizione della soluzione 
Per quanto riguarda la soluzione proposta, prima di iniziare la computazione vera e propria del gioco tutti i processi inizializzano le strutture necessarie per la risoluzione del gioco e il corretto funzionamento. La matrice <code> matrix</code> rappresenta il mondo iniziale generato randomicamente, la matrice <code>finalWorld</code>  conterrà il mondo al termine delle generazioni, oltre a diversi array e altri tipi di dati utili per l’invio e la ricezione delle righe ai processi adiacenti sia per la successiva fase di computazione.Per quanto riguarda l’invio dei dati si è scelto di suddividere la matrice iniziale per righe, così da inviare ad ogni processo, incluso il <code>Master</code>, un insieme di dati che verranno letti da posizioni contigue di memoria. Inoltre, questa scelta è stata fatta anche per agevolare la fase di “ricostruzione” delle sottomatrici così da lavorare in una maniera array-oriented.
Oltre queste scelte, è stato sfruttato il concetto di comunicator per fare in modo di evitare che il programma si blocchi una volta terminata la computazione quando il numero di processi supera il numero di righe. Sono stati creati due nuovi gruppi <code>world_group e prime_group </code> i quali rispettivamente indicano il gruppo che verrà usato per la creazione del comunicator se il numero di processi non supera il numero di righe e se il numero di processi supera il numero di righe. Se dovesse verificarsi questa seconda ipotesi, allora verrà creato il comunicator <code>prime_comm</code> con numero massimo di processi uguale al numero di righe se invece non viene inserito un numero superiore di processi rispetto al numero di righe verrà creato un nuovo communicator <code>prime_comm</code> sulla base però del <code>world_group</code> usando come numero di processi il numero di processi dato in input,poichè minore o uguale al numero di righe

```C
int main(int argc, char *argv[]) {
    int rank, size;  //Rank processo e numero processi
    double end_time, start_time;
    MPI_Request requestTop, requestBottom;  //request per le chiamate delle funzione wait
    MPI_Status status;

    //Funzioni necessarie al funzionamento
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /*Ottengo i parametri necessari per iniziare la computazione*/
    int ROW_SIZE = atoi(argv[1]);    //Numero righe matrice
    int COL_SIZE = atoi(argv[2]);    //Numero colonne matrice
    int GENERATION = atoi(argv[3]);  //Numero generazioni da eseguire

    int master = 0;  //Variabile che conterrà il rank del processo Master

    int generazioni = 0;  //Variabile che conterrà il numero delle generazioni passate nonché la corrente
    int partecipants;     //Numero di processi che lavorano
    char *matrix;         //Matrice di char che conterrà il mondo
    int *send_counts;     //Array che conterrà il numero di elementi per ogni processo
    int *displacements;   //Array che conterrà l'offset tra i diversi processi
    int *rowPerProcess;   //Array che conterrà il numero di righe per processo
    char *rowToSend, *rowToSendNext;
    int resto;
    int divisione;
    char *topRow, *bottomRow;   //Array per la rigaTop e rigaBottom ricevute
    char *matrixrec;            //Matrice che verrà inviata ad ogni processo
    char *newMiniWorld;         //Matrice che verrà utilizzata per aggiornare il mondo di ogni processo
    char *newMatrix;            //Matrice nuova con l'aggiunta delle righe ricevute dai vicini
    char *finalWorld;           //Matrice che conterrà il mondo finale al termine del numero di generazioni
    

    partecipants = size > ROW_SIZE ? ROW_SIZE : size;     //Numero di partecipanti
    int next = (rank + 1) % partecipants;                 //Calcolo il processo successivo
    int prev = (rank + partecipants - 1) % partecipants;  //Calcolo il processo precedente

    MPI_Group world_group, prime_group;            //Dichiaro due nuovi gruppi world_group e prime_group
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);  //Creo un nuovo gruppo(world_group) usando il communicator MPI_COMM_WORLD
    MPI_Comm prime_comm;
    if (size > ROW_SIZE) {
        int numero = size > ROW_SIZE ? (size + ROW_SIZE) - size : size;
        int ranks[numero];
        for (int i = 0; i < numero; i++) {
            printf("Popolo\n");
            ranks[i] = i;
        }
        //Creo un nuovo gruppo(prime_group) formato dai processi contenuti nell'array rank, dove il processo i nel nuovogruppo è il processo con rank ranks[i] nel gruppo
        MPI_Group_incl(world_group, numero, ranks, &prime_group);

        //Creo un nuovo comunicator (prime_comm)
        MPI_Comm_create(MPI_COMM_WORLD, prime_group, &prime_comm);
    } else {
        //Nel caso in cui il numero di processi non supera il numero di righe, creo il communicatore sulla base del world_group, quindi senza modifiche
        MPI_Comm_create(MPI_COMM_WORLD, world_group, &prime_comm);
    }
    //Processi in eccesso 
    if (prime_comm == MPI_COMM_NULL) {
        printf("I'm here and I'll go away\n");
        MPI_Finalize();
        exit(0);
    }

    MPI_Comm_rank(prime_comm, &rank);
    MPI_Comm_size(prime_comm, &size);
    //printf("Rank: %d/%d\n", rank, size);
    start_time = MPI_Wtime();
    MPI_Barrier(prime_comm);
```

Una volta ultimata la fase di inizializzazione, il processo Master si occupa di generare casualmente la matrice, per la prima generazione, con la funzione <code> generateAndPrintMatrix() </code> : questa funzione come si evince dal nome, permette di generare, in maniera casuale, una matrice di char di dimensione NXM la quale conterrà a (cella viva) d (cella morta).

```C
 if (rank == master) {
              matrix = (char *)malloc(sizeof(char) * (ROW_SIZE * COL_SIZE));
              generateAndPrintMatrix(matrix, ROW_SIZE, COL_SIZE, GENERATION);
              finalWorld = malloc(sizeof(char) * (ROW_SIZE * COL_SIZE));
        }
```

Successivamente, vengono calcolati il resto e il risultato della divisione così da poter assegnare correttamente le righe ai processi. Nel caso in cui la divisione abbia resto diverso da 0, si assegnerà una riga in più ai processi con rank< resto altrimenti si riuscirà ad assegnare correttamente ed equamente a tutti lo stesso numero di righe. Inoltre, viene allocato spazio per diversi tipi di dati come <code>send_counts, displacements e rowPerProcess</code> che contengono rispettivamente il numero di elementi per ogni processi, i displacements tra ogni processo e il numero di righe di ogni processo, i quali verranno calcolati tramite la funzione <code> initDisplacementPerProcess(). </code>

```C
        resto = (ROW_SIZE) % (partecipants);
        divisione = (ROW_SIZE) / (partecipants);

        
        send_counts = malloc(sizeof(int) * partecipants);    //Alloco spazio per array che conterrà gli elementi per ogni processo
        displacements = malloc(sizeof(int) * partecipants);   //Alloco spazio per array che conterrà i displacements per ogni processo
        rowPerProcess = malloc(partecipants * sizeof(int));  //Alloco spazio per array che conterrà il numero di righe per ogni processo
        int dimNewMatrix = 0;
       //Chiamata funzione di inizializzazione per displacement e send_counts per i processi
        initDisplacementPerProcess(send_counts, displacements, rowPerProcess, partecipants, resto, divisione, COL_SIZE);
        
        matrixrec = malloc(sizeof(char) * (rowPerProcess[rank] * COL_SIZE));      //Alloco spazio per la matrice di ricezione
        dimNewMatrix = rowPerProcess[rank] * COL_SIZE;                            //Calcolo dimensione della matrice da ricevere
        newMiniWorld = malloc(sizeof(char) * (rowPerProcess[rank]) * COL_SIZE);   //Alloco spazio per la matrice dei processi aggiornata
        newMatrix = malloc(sizeof(char) * (rowPerProcess[rank] + 2) * COL_SIZE);  //Alloco spazio per la matrice che verrà generata dopo la ricezione delle righe
        topRow = malloc(sizeof(char) * COL_SIZE);                                 //Inizializzo topRow che ricevo
        bottomRow = malloc(sizeof(char) * COL_SIZE);                              //Inizalizzo bottomRow che ricevo
        rowToSend = malloc(COL_SIZE * sizeof(char));                              //Inizializzo la riga da inviare al precedente
        rowToSendNext = malloc(COL_SIZE * sizeof(char));                          //Inizializzo la riga da inviare al successivo
```


Una volta completata la fase d'inizializzazione e dopo aver allocato spazio anche per i tipi di dati che permetteranno l’invio delle righe <code> rowToSend, rowToSendNext, topRow, bottom Row</code> nonchè le matrici <code>matrixrec, newMatrix e newMiniWorld</code> viene svolto un ciclo <code>while</code> il quale termina una volta raggiunto il numero massimo di generazioni. All’interno del ciclo se si tratta della prima generazione viene fatta una <code>MPI_Scatterv()</code> la quale permetterà l’invio delle righe ad ogni processo. Le righe inviate verranno contenute localmente in <code>matrixRec</code>. Una volta ricevute le righe, i processi inizieranno a comunicare con i processi adiacenti per l’invio delle loro righe estreme (prima e ultima) da inviare rispettivamente al processo precedente e al successivo. Tutto questo viene fatto in maniera non bloccante con la funzione <code> ISend</code> per l’invio e <code>Recv</code> per la ricezione. Una volta ricevute le righe, attraverso la funzione <code>rebuildMatrixAfterRec()</code> viene “ricostruita” una nuova sottomatrice <code>newMatrix</code> per il processo corrente con l’aggiunta delle righe ricevute, rispettivamente in prima e ultima posizione, in questo modo si può proseguire con la chiamata di <code>gameOfLifeUpdate()</code> che permetterà l’aggiornamento della porzione di mondo contenuta dal processo, usando le ghost cells (righe ricevute) ma non includendole nella computazione, altrimenti questo altererebbe il risultato. L’aggiornamento permetterà di capire se una cella viva (a) potrà sopravvivere o morire a seguito di under oppure di over population mentre per una cella morta (d) permetterà di vedere se potrà riprodursi o meno. Al termine di questa fase, lo stato aggiornato sarà contenuto nella matrice <code>newMiniWorld</code> la quale verrà assegnata a <code>matrixRec</code> cosicché il ciclo possa continuare con l’invio di nuove righe corrette, dopo una fase di computazione. Al termine di questo viene incrementato il contatore delle generazioni.

```C
while (generazioni < GENERATION) {
            
            if (generazioni == 0) {
                
                MPI_Scatterv(matrix, send_counts, displacements, MPI_CHAR, matrixrec, dimNewMatrix, MPI_CHAR, 0, prime_comm);
            }


            int rows = rowPerProcess[rank];  //Numero righe processo rank

            memcpy(rowToSend, &matrixrec[0], COL_SIZE * sizeof(char));  //Copio in rowToSend la prima riga della sottomatrici
            MPI_Isend(rowToSend, COL_SIZE, MPI_CHAR, prev, 11, prime_comm, &requestTop);        //Invio la riga al mio prev
            memcpy(rowToSendNext, &matrixrec[(rows - 1) * COL_SIZE], COL_SIZE * sizeof(char));  //Copio in rowToSendNext l'ultima riga della sottomatric
            MPI_Isend(rowToSendNext, COL_SIZE, MPI_CHAR, next, 11, prime_comm, &requestBottom);  //Invio la riga al mio next

            //DONE: Recv
            MPI_Recv(bottomRow, COL_SIZE, MPI_CHAR, prev, 11, prime_comm, &status);  //Ricevo la riga dal mio prec

            MPI_Recv(topRow, COL_SIZE, MPI_CHAR, next, 11, prime_comm, &status);  //Ricevo la riga dal mio next

            if (prev == next) {
                //Funzione di ricostruzione della sottomatrice, se ho due processi o 1, next e prev coincidono quindi
                //la chiamata è uguale alla successiva ma si invertono topRow e bottomRow
                rebuildMatrixAfterRec(matrixrec, (send_counts[rank] / rows), rows, topRow, bottomRow, newMatrix, COL_SIZE);

            } else {
                //Stessa chiamata della funzione precedente ma si invertono bottomRow e topRow per popolare correttamente
                rebuildMatrixAfterRec(matrixrec, (send_counts[rank] / rows), rows, bottomRow, topRow, newMatrix, COL_SIZE);
            }

            //Chiamo la funzione principale per aggiornare le matrici
            gameOfLifeUpdate(newMatrix, newMiniWorld, rows + 1, 1, rank, COL_SIZE);

            //Aggiorno matrixrec, cosi che le righe possano essere inviate correttamente per le generazioni successive
            matrixrec = newMiniWorld;

            generazioni++;
        }

        //Restituisco la matrice finale contenuta in finalWorld al master per la stampa
        MPI_Gatherv(newMiniWorld, send_counts[rank], MPI_CHAR, finalWorld, send_counts, displacements, MPI_CHAR, master, prime_comm);
    }
```


Dopo aver completato tutte le generazioni, tramite la funzione <code>MPI_Gatherv()</code> è stato possibile prendere tutti i <code>newMiniWorld</code> e aggregarli in <code>finalWorld</code>, in questo modo lo stato finale del gioco sarà contenuto al suo interno e sarà possibile stamparlo.


# Funzioni principali
In questa sezione verranno descritte in dettagli le funzioni principali del programma

La funzione seguente <code>gameOfLifeUpdate</code> rappresenta il core del programma. Essa viene chiamata da ogni processo
per effettuare l'aggiornamento della loro porzione di matrice precedentemente ricevuta.


```C
void gameOfLifeUpdate(char *newMatrix, char *newMiniWorld, int numbersOfRows, int flag, int rank, int COL_SIZE) {
    //la variabile flag indica da dove deve partire la computazione usando ma non computando le ghost cells
    //Numberofrows indica il punto dove deve finire la computazione evitando le ghost cells

    for (int i = flag; i < numbersOfRows; i++) {
        
        for (int j = 0; j < COL_SIZE; j++) {
            //Se la cella è viva, allora controllo se deve morire o sopravvivere alla generazione successiva
            if (newMatrix[j + (i * COL_SIZE)] == ALIVE) {
                //sottraendo flag ad i ottengo la posizione corretta di dove inserire nel newMiniWorld
                newMiniWorld[j + ((i - flag) * COL_SIZE)] = checkUnderAndOverPopulation(newMatrix, i, j, numbersOfRows + 1, COL_SIZE, COL_SIZE);
               
                //Se la cella è morta allora controllo se deve riprodursi
            } else if (newMatrix[j + (i * COL_SIZE)] == DEAD) {
                //sottraendo flag ad i ottengo la posizione corretta di dove inserire nel newMiniWorld
                newMiniWorld[j + ((i - flag) * COL_SIZE)] = checkReproduction(newMatrix, i, j, numbersOfRows + 1, COL_SIZE, COL_SIZE);
                
            }
        }
    }
}
```
Le seguenti due funzioni: <code>checkReproduction e checkUnderAndOverPopulation</code> sono quelle che verranno chiamate dalla funzione <code>gameOfLifeUpdate</code> per effettuare i controlli nel caso in cui la cella sia morta o viva, andando quindi a verificare se la cella potrà riprodursi, sopravvivere alla generazione successiva oppure morire per under o over population

## checkReproduction

```C
char checkReproduction(char *matrixrec, int currentRow, int currentColumn, int numbersOfRows, int numbersOfCols, int COL_SIZE) {
    int viciniVivi = 0;

    
    if (currentRow > 0) {
        viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
    }
    
    if (currentRow < numbersOfRows - 1) {
        viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
    }

   
    if (currentColumn > 0)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
   
    if (currentColumn < numbersOfCols - 1)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;

    //Se il numero di viciniVivi è esattamente 3 la cella si riproduce altrimenti resta morta
    return (viciniVivi == 3) ? ALIVE : DEAD;
    
}
```

## checkUnderAndOverPopulation

```C
char checkUnderAndOverPopulation(char *matrixrec, int currentRow, int currentColumn, int numbersOfRows, int numbersOfCols,int COL_SIZE) {
    int viciniVivi = 0;
    
    if (currentRow > 0) {
        viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
       
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
       
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
    }
    
    if (currentRow < numbersOfRows - 1) {
        viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
    }
   
    if (currentColumn > 0)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;

    if (currentColumn < numbersOfCols - 1)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;

 
    return (viciniVivi > 3 || viciniVivi < 2) ? DEAD : ALIVE;   

}
```

# How to run 
<code>mpicc</code> NomeFile.c -o NomeFile.out 

<code>mpirun -np {VCPUs} NomeFile.out {Numero righe}, {Numero colonne}, {generazioni} </code> 
# Correctness
  Matrice di input appositamente creata per mostrare il funzionamento delle quattro condizioni di gioco. 
  
    1. Qualsiasi cella viva con meno di due celle vive adiacenti muore, come per effetto d'isolamento (underpopulation)
    2. Qualsiasi cella viva con due o tre celle vive adiacenti sopravvive alla generazione successiva;
    3. Qualsiasi cella viva con più di tre celle vive adiacenti muore, come per effetto di sovrappopolazione (overpopulation)
    4. Qualsiasi cella morta con esattamente tre celle vive adiacenti diventa una cella viva, come per effetto di riproduzione (reproduction)
![image](https://user-images.githubusercontent.com/55912466/118352929-67565400-b564-11eb-9127-fca585af2730.png)![image](https://user-images.githubusercontent.com/55912466/118353154-5b1ec680-b565-11eb-8a01-489e052c17b2.png)  ![image](https://user-images.githubusercontent.com/55912466/118352980-a684a500-b564-11eb-9359-5d009a8ac8ce.png) 
![image](https://user-images.githubusercontent.com/55912466/118353033-e2b80580-b564-11eb-9976-3d564b448155.png)  ![image](https://user-images.githubusercontent.com/55912466/118353040-ec416d80-b564-11eb-95f1-0674e717fe5e.png)![image](https://user-images.githubusercontent.com/55912466/118353056-fe231080-b564-11eb-903f-2f54fcd06468.png) 
![image](https://user-images.githubusercontent.com/55912466/118353070-12ffa400-b565-11eb-8142-9cc6774d898d.png)  ![image](https://user-images.githubusercontent.com/55912466/118353077-1b57df00-b565-11eb-9c5b-51a5581ff35c.png)![image](https://user-images.githubusercontent.com/55912466/118353090-24e14700-b565-11eb-9d19-2416f392e03d.png) ![image](https://user-images.githubusercontent.com/55912466/118353123-404c5200-b565-11eb-896e-e0df99badf57.png)

# Benchmarks 
I test per valutare le prestazioni dell'algoritmo sono stati effettuati su un cluster AWS composto da 8 istanze di tipo m4.large. Inoltre, per valutare i test verranno considerati la <code>scalabilità forte</code> <code>scalabilità debole</code> e lo <code>speed-up</code> andando poi a valutare l'efficienza delle varie esecuzioni con diverso numero di processori.

## Strong Scalability
La scalabilità forte è stata testata eseguendo il codice con un differente numero di processori (VCPUs) su una matrice di dimensioni costanti, effettuando diverse rilevazioni di tempo andando poi a calcolare un tempo medio per ogni esecuzione con i diversi processori.

## Weak Scalability 
La scalabilità debole è stata misurata eseguendo il codice con un differente numero di VCPUs e aumentando di 1000 ogni volta il numero di righe, tenendo costante il numero di colonne. Anche qui sono state effettuate diverse rilevazioni calcolando poi la media per ogni esecuzione con i diversi processori.
L'efficienza della weak scalability è stata calcolata tramite la seguente formula: <code> Ep = T1/Tp </code> 

## Speed-up
Lo speedup misura la riduzione del tempo di  esecuzione dell’algoritmo eseguito su p processori rispetto all’esecuzione su 1 processore. Lo spee-up in generale è minore del numero di processori mentre lo speed-up ideale assume proprio valore p. Quindi un esecuzione su 2 processori presenta uno speed-up ideale di 2 e cosi via.
    Lo speed-up è per la strong scalability è stato calcolato mediante la seguente formula: <code>Sp = T1 /Tp </code> dove T1 è il tempo d'esecuzione su un singolo processore mentre Tp è il tempo d'esecuzione dell'algoritmo con p processori.
# Test 1 
#### Rows=1000 Columns=1000 Generation=100

| Instances | VCPUs | Time |
| --------- | ----- | ---- |
| 1 | 1 | 3,2839828 |
| 1 | 2 | 2,27761652 |
| 2 | 4 | 1,4547171 |
| 3 | 6 | 0,9828964 |
| 4 | 8 | 0,7543306 |
| 5 | 10 | 0,6232448 |
| 6 | 12 | 0,5279104 |
| 7 | 14 | 0,4657618 |
| 8 | 16 | 0,4196014 |

| VCPUs | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
| ----- | - | - | - | - | -- | -- | -- | -- |
| Speed-up | 1,18 | 2,25 | 3,34 | 4,35 | 5,26 | 6,22 | 7,05 |7,82
| Efficienza | 59,14% | 56,43% | 55,68% | 54,41% | 52,69% | 51,83% | 50,36% | 48,91% |
  
| ![image](https://user-images.githubusercontent.com/55912466/119875867-8cb16d80-bf27-11eb-82e4-854882953cae.png) | ![image](https://user-images.githubusercontent.com/55912466/120164400-d5547980-c1fa-11eb-9064-7c802219a22a.png) |
| ---------------------------------------------------------------------------------------------------------------- | --------------------|

## Risultati 
Come si può notare dalla tabella riassuntiva e dai grafici, lo speed-up su 2 processori è il "più vicino" allo speed-up ideale. Quindi l'algoritmo parallelo su due processori è quello più veloce in rapporto alle risorse utilizzate. Questo lo si nota anche dal rapporto tra la speed-up e il numero di processori, il quale rappresenta l'efficienza e quindi una misura dello sfruttamento delle risorse di calcolo. Infatti, all'aumentare del numero di processori lo speedup si allontana sempre di più da quello ideale comportando anche una perdita d'efficienza. L'efficienza in questo test case si assesta tra il 59% e il 48% diminuendo all'aumentare del numero di processori, quindi lo sfruttamento del parallelismo del calcolatore da parte dell'algoritmo dimininuisce nonostante aumenti il numero di processori a disposizione,anche perché la comunicazione su macchine diverse, dopo 2 VCPUs, è più dispendiosa rispetto ad una comunicazione sulla stessa macchina.

# Test 2
#### Rows=3000 Columns=3000 Generations=100
| Instances | VCPUs | Time |
| --------- | ----- | ---- |
| 1 | 1 | 28,538899 |
| 1 | 2 | 25,1866472 |
| 2 | 4 | 12,7318122 |
| 3 | 6 | 8,7138484 | 
| 4 | 8 | 6,6999046 |
| 5 | 10 | 5,4887132|
| 6 | 12 | 4,6068862|
| 7 | 14 | 4,02782214 |
| 8 | 16 | 3,5953646 |

| VCPUs | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
| ----- | - | - | - | - | -- | -- | -- | -- |
| Speed-up | 1,13 | 2,24 | 3,27 | 4,26 | 5,19 | 6,2 | 7,08 |7,93
| Efficienza | 56,65% | 56,03% | 54,58% | 53,24% | 51,32% | 51,62% | 50,61% | 49,61% |

| ![image](https://user-images.githubusercontent.com/55912466/119875627-42c88780-bf27-11eb-94bc-2c8986f311fd.png) | ![image](https://user-images.githubusercontent.com/55912466/120163796-2f087400-c1fa-11eb-976f-81210529c78f.png) |
| ---------------------------------------------------------------------------------------------------------------- | --------------------|

## Risultati 
Come si può notare dalla tabella riassuntiva e dai grafici, lo speed-up su 2 processori è il "più vicino" allo speed-up ideale. Quindi l'algoritmo parallelo su due processori è quello più veloce in rapporto alle risorse utilizzate. Questo lo si nota anche dal rapporto tra la speed-up e il numero di processori, il quale rappresenta l'efficienza e quindi una misura dello sfruttamento delle risorse di calcolo. Infatti, all'aumentare del numero di processori lo speedup si allontana sempre di più da quello ideale comportando anche una perdita d'efficienza. In questo test case l'efficienza si assesta tra il 56% e il 49% diminuendo, anche in questo caso,all'aumentare del numero di processori. Questo fa capire che lo sfruttamento del parallelismo del calcolatore da parte dell'algoritmo dimininuisce nonostante aumenti il numero di processori a disposizione,anche perché la comunicazione su macchine diverse, dopo 2 VCPUs, è più dispendiosa rispetto ad una comunicazione sulla stessa macchina.

# Test 3 
#### Rows=6000 Columns=6000 Generations=100
| Instances | VCPUs | Time |
| --------- | ----- | ---- |
| 1 | 1 | 118,763964 |
| 1 | 2 | 99,993672 |
| 2 | 4 | 51,1842902 |
| 3 | 6 | 34,7227854 | 
| 4 | 8 | 26,6233892 |
| 5 | 10 | 21,8550922|
| 6 | 12 | 18,604946 |
| 7 | 14 | 16,082835 |
| 8 | 16 | 14,4113246 |

| VCPUs | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
| ----- | - | - | - | - | -- | -- | -- | -- |
| Speed-up | 1,18 | 2,32 | 3,42 | 4,46 | 5,43 | 6,38 | 7,38 |8,24
| Efficienza | 60,38% | 58% | 57% | 55,76% | 54,34% | 53,19% | 52,74% | 51,50% |

| ![image](https://user-images.githubusercontent.com/55912466/119876304-09444c00-bf28-11eb-87d6-689787efd191.png) | ![image](https://user-images.githubusercontent.com/55912466/120162467-b48b2480-c1f8-11eb-9629-f076ed4128cb.png) |
| ---------------------------------------------------------------------------------------------------------------- | --------------------|


## Risultati 
Come si può notare dalla tabella riassuntiva e dai grafici, lo speed-up su 2 processori è il "più vicino" allo speed-up ideale. Quindi l'algoritmo parallelo su due processori è quello più veloce in rapporto alle risorse utilizzate. Questo lo si nota anche dal rapporto tra la speed-up e il numero di processori, il quale rappresenta l'efficienza e quindi una misura dello sfruttamento delle risorse di calcolo. Infatti, all'aumentare del numero di processori lo speedup si allontana sempre di più da quello ideale comportando anche una perdita d'efficienza. Anche se in questo caso l'efficienza si assesta tra il 60% e il 50%. Questo fa capire che lo sfruttamento del parallelismo del calcolatore da parte dell'algoritmo dimininuisce nonostante aumenti il numero di processori a disposizione, anche perché la comunicazione su macchine diverse, dopo 2 VCPUs, è più dispendiosa rispetto ad una comunicazione sulla stessa macchina
# Weak Scalability
Il seguente grafico riporta il test della <code>Weak Scalability </code> il quale è stato fatto, aumentando per ogni nuovo processore il numero di righe,aggiungendo un valore di 1000. 
| VCPUs | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
| ----- | - | - | - | - | -- | -- | -- | -- |
| Efficienza | 58% | 56% | 55% | 54% | 53% | 53% | 51% | 50%

![image](https://user-images.githubusercontent.com/55912466/120096467-0154f980-c12c-11eb-9ba1-5b7659917811.png)
