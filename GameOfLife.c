include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/*
    Name: Felice
    Surname:Coppola
    Matricola:0522501020
    My String: FELICECOPPOLA
    
    1. game of life	Conway’s Game of Life Game
    2. Amazon EC2 Instance m4.large
    mpirun --allow-run-as-root --mca btl_vader_single_copy_mechanism none -np 2 ./FeliceCLI.out 3000 3000 100
*/
//Definizioni costanti
#define ALIVE 'a'
#define DEAD 'd'

//Funzioni di stampa
void printAlive(char c) {
    printf("\033[0;32m%2c\033[0m ", c);
}

void printDead(char c) {
    printf("\033[0;31m%2c\033[0m ", c);
}

void printMatrix2(char *matrix, int rows, int COL_SIZE) {
    for (int i = 0; i < rows; i++) {
        printf("Riga:%2d[ ", i);
        for (int j = 0; j < COL_SIZE; j++) {
            {
                if (matrix[j + (i * COL_SIZE)] == ALIVE)

                    printAlive(matrix[j + (i * COL_SIZE)]);
                else if (matrix[j + (i * COL_SIZE)] == DEAD)
                    printDead(matrix[j + (i * COL_SIZE)]);
            }
        }
        printf("]\n");
    }
}

//Funzioni di Inizializzazione matrice e sottomatrici
void generateAndPrintMatrix(char *matrix, int ROW_SIZE, int COL_SIZE, int GENERATION) {
    printf("Sono il Master e inizializzo randomicamente la matrice\n");
    srand(time(NULL) * (ROW_SIZE + COL_SIZE));
    for (int i = 0; i < ROW_SIZE; i++) {
        for (int j = 0; j < COL_SIZE; j++) {
            if (rand() % 2 == 0) {
                matrix[j + (i * COL_SIZE)] = DEAD;
            } else
                matrix[j + (i * COL_SIZE)] = ALIVE;
        }
    }
    /*printf("Stampo la matrice\n");
    for (int i = 0; i < ROW_SIZE; i++) {
        printf("[");
        for (int j = 0; j < COL_SIZE; j++) {
            {
                if (matrix[j + (i * COL_SIZE)] == ALIVE)
                    printAlive(matrix[j + (i * COL_SIZE)]);
                else if (matrix[j + (i * COL_SIZE)] == DEAD)
                    printDead(matrix[j + (i * COL_SIZE)]);
            }
        }
        printf("]\n");
    }*/
}
//Funzione di inizializzazione
void initDisplacementPerProcess(int send_counts[], int displacements[], int rowPerProcess[], int partecipants, int resto, int divisione, int COL_SIZE) {
    for (int i = 0; i < partecipants; i++) {
        rowPerProcess[i] = (i < resto) ? divisione + 1 : divisione;                 //calcolo il numero di righe per processo, se riesce equamente altrimenti una riga in più ai processi con rank<resto
        send_counts[i] = (rowPerProcess[i]) * COL_SIZE;                             //calcolo il numero di elementi per ogni processo in base al numero di righe
        displacements[i] = i == 0 ? 0 : displacements[i - 1] + send_counts[i - 1];  //calcolo il displacements tra gli elementi dei processi
        //printf("Displ[%d] e send_counts[%d] \n", i, send_counts[i]);
    }
}

//Funzione che permette di ricostruire le sottomatrici una volta ricevute le due righe (rowRec e rowRecPrev)
void rebuildMatrixAfterRec(char *matrixrec, int offset, int rows, char rowRec[], char rowRecPrev[], char *newMatrix, int COL_SIZE) {
    int step = 0;

    for (int i = 0; i < 1; i++) {
        for (int j = step, z = 0; j < step + COL_SIZE; j++, z++) {
            newMatrix[j] = rowRec[z];
        }
        step = step + COL_SIZE;
    }

    int step1 = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = step1, z = step; j < step1 + COL_SIZE && z < step + COL_SIZE; j++, z++) {
            newMatrix[z] = matrixrec[j];
        }
        step = step + COL_SIZE;
        step1 = step1 + COL_SIZE;
    }
    for (int i = rows + 1; i < rows + 2; i++) {
        for (int j = step, z = 0; j < step + COL_SIZE; j++, z++) {
            newMatrix[j] = rowRecPrev[z];
        }
    }
}

//Funzione che  permette di fare il check di over and under population
char checkUnderAndOverPopulation(char *matrixrec, int currentRow, int currentColumn, int numbersOfRows, int numbersOfCols, int COL_SIZE) {
    int viciniVivi = 0;

    if (currentRow > 0) {
        viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        //Ho il vicino sinistro
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        //Ho il vicino destro
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
    }
    //Controllo con la riga sotto
    if (currentRow < numbersOfRows - 1) {
        viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        //Ho il vicino sinistro
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
        //Ho il vicino destro
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;
    }
    //Se non sono presenti vicini top o bottom, controlliamo right or left
    if (currentColumn > 0)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn - 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;

    if (currentColumn < numbersOfCols - 1)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn + 1)] == ALIVE) ? viciniVivi + 1 : viciniVivi;

    //OverPopulation or UnderPopulation
    return (viciniVivi > 3 || viciniVivi < 2) ? DEAD : ALIVE;
}
//Funzione che  permette di controllare se una cella può riprodursi
char checkReproduction(char *matrixrec, int currentRow, int currentColumn, int numbersOfRows, int numbersOfCols, int COL_SIZE) {
    int viciniVivi = 0;

    //Se la riga ha un indice maggiore di 0 ha sicuramente un vicino sopra di lei
    if (currentRow > 0) {
        viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        //Se la colonna ha un indice maggiore di 0 sicuramente ha un vicino sinistro, Left neigh
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        //Se la colonna non è l'ultima ha sicuramente un vicino destro, Right neigh
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow - 1) * COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
    }
    //Se la riga è minore del numero di righe-1, vuol dire che non è l'ultima ed ha un vicino sotto di lei
    if (currentRow < numbersOfRows - 1) {
        viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        //Se la colonna ha un indice maggiore di 0 sicuramente ha un vicino sinistro, Left neigh
        if (currentColumn > 0)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
        //Se la colonna non è l'ultima ha sicuramente un vicino destro, Right neigh
        if (currentColumn < numbersOfCols - 1)
            viciniVivi = (matrixrec[((currentRow + 1) * COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
    }
    //Se non sono presenti vicini top o bottom, controlliamo right or left

    //Controllo vicino sinistro
    if (currentColumn > 0)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn - 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;
    //Controllo vicino destro
    if (currentColumn < numbersOfCols - 1)
        viciniVivi = (matrixrec[((currentRow)*COL_SIZE) + (currentColumn + 1)]) == ALIVE ? viciniVivi + 1 : viciniVivi;

    //Se il numero di viciniVivi è esattamente 3 la cella si riproduce altrimenti resta morta
    return (viciniVivi == 3) ? ALIVE : DEAD;
}
//Funzione core che aggiorna correttamente il mondo di ogni processo
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

    int master = 0;       //Variabile che conterrà il rank del processo Master
    int generazioni = 0;  //Variabile che conterrà il numero delle generazioni passate nonché la corrente
    int partecipants;     //Numero di processi che lavorano
    char *matrix;         //Matrice di char che conterrà il mondo
    int *send_counts;     //Array che conterrà il numero di elementi per ogni processo
    int *displacements;   //Array che conterrà l'offset tra i diversi processi
    int *rowPerProcess;   //Array che conterrà il numero di righe per processo
    char *rowToSend, *rowToSendNext;
    int resto;
    int divisione;
    char *topRow, *bottomRow;  //Array per la rigaTop e rigaBottom ricevute
    char *matrixrec;           //Matrice che verrà inviata ad ogni processo
    char *newMiniWorld;        //Matrice che verrà utilizzata per aggiornare il mondo di ogni processo
    char *newMatrix;           //Matrice nuova con l'aggiunta delle righe ricevute dai vicini
    char *finalWorld;          //Matrice che conterrà il mondo finale al termine del numero di generazioni

    partecipants = size > ROW_SIZE ? ROW_SIZE : size;     //Numero di partecipanti
    int next = (rank + 1) % partecipants;                 //Calcolo il processo successivo
    int prev = (rank + partecipants - 1) % partecipants;  //Calcolo il processo precedente

    MPI_Group world_group, prime_group;            //Dichiaro due nuovi gruppi world_group e prime_group
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);  //Creo un nuovo gruppo(world_group) usando il communicator MPI_COMM_WORLD
    MPI_Comm prime_comm;                           //Dichiaro il nuovo communicator
    if (size > ROW_SIZE) {
        int numero = size > ROW_SIZE ? ROW_SIZE : size;
        int ranks[numero];
        for (int i = 0; i < numero; i++) {
            ranks[i] = i;
        }
        //Creo un nuovo gruppo(prime_group) formato dai processi contenuti nell'array rank, dove il processo i nel nuovogruppo è il processo con rank ranks[i] nel gruppo
        MPI_Group_incl(world_group, numero, ranks, &prime_group);

        //Creo un nuovo comunicator (prime_comm) sulla base del gruppo prime_group il quale è un sottoinsieme del gruppo del comm MPI_COMM_WORLD
        MPI_Comm_create(MPI_COMM_WORLD, prime_group, &prime_comm);
    } else {
        //Nel caso in cui il numero di processi non supera il numero di righe, creo il communicatore sulla base del world_group, quindi senza modifiche
        MPI_Comm_create(MPI_COMM_WORLD, world_group, &prime_comm);
    }
    //Rank che non fanno parte della comunicazione vengono eliminati
    if (prime_comm == MPI_COMM_NULL) {
        MPI_Finalize();
        exit(0);
    }

    MPI_Comm_rank(prime_comm, &rank);
    MPI_Comm_size(prime_comm, &size);
    //printf("Rank: %d/%d\n", rank, size);
    start_time = MPI_Wtime();
    MPI_Barrier(prime_comm);
    if (rank < partecipants) {
        //Il master genera il mondo iniziale e alloca spazio per quello finale
        if (rank == master) {
            matrix = (char *)malloc(sizeof(char) * (ROW_SIZE * COL_SIZE));
            generateAndPrintMatrix(matrix, ROW_SIZE, COL_SIZE, GENERATION);
            finalWorld = malloc(sizeof(char) * (ROW_SIZE * COL_SIZE));
        }

        resto = (ROW_SIZE) % (partecipants);
        divisione = (ROW_SIZE) / (partecipants);

        send_counts = malloc(sizeof(int) * partecipants);    //Alloco spazio per array che conterrà gli elementi per ogni processo
        displacements = malloc(sizeof(int) * partecipants);  //Alloco spazio per array che conterrà i displacements per ogni processo
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

        while (generazioni < GENERATION) {
            //Se è la prima generazione, invio ai processi la loro porzione. Successivamente alla prima tutto verrà fatto automaticamente
            if (generazioni == 0) {
                //Ho modificato matrixrec e count al suo fianco, bypassando rebuildMatriX
                MPI_Scatterv(matrix, send_counts, displacements, MPI_CHAR, matrixrec, dimNewMatrix, MPI_CHAR, 0, prime_comm);
            }

            int rows = rowPerProcess[rank];  //Numero righe processo rank

            memcpy(rowToSend, &matrixrec[0], COL_SIZE * sizeof(char));                           //Copio in rowToSend la prima riga della sottomatrice
            MPI_Isend(rowToSend, COL_SIZE, MPI_CHAR, prev, 11, prime_comm, &requestTop);         //Invio la riga al mio prev
            memcpy(rowToSendNext, &matrixrec[(rows - 1) * COL_SIZE], COL_SIZE * sizeof(char));   //Copio in rowToSendNext l'ultima riga della sottomatrice
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

    MPI_Barrier(prime_comm);
    end_time = MPI_Wtime();
    //libero spazio
    free(rowPerProcess);
    free(send_counts);
    free(displacements);
    free(matrixrec);
    free(newMatrix);
    //free(newMiniWorld);
    free(topRow);
    free(bottomRow);

    //Fine della computazione

    MPI_Finalize();
    if (rank == master) {
        //printMatrix2(finalWorld, ROW_SIZE, COL_SIZE);
        free(finalWorld);
        free(matrix);

        printf("⏱ Time in ms=%f\n", end_time - start_time);
    }

    return 0;
}
