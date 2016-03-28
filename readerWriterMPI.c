/*
 *  readerWriterMPI.c
 *
 *  Trabalho de implementação da disciplina Sistemas Distribuídos
 *
 *  Criado em: 2016
 *  	Autores: Leonardo Ramos, Carlos de Oliveira, Bruno Moraes
 *  
 *  Compilar:
 *		mpicc readerWriterMPI.c -o readerWriterMPI
 *
 *  Executar:
 *		mpirun -np <NUMERO_DE_PROCESSOS> ./readerWriterMPI	
 *  
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <mpi.h>
  
#define MAXPROC 1000

main (int argc, char **argv) {
	int readerStartIndex = 0, writerStartIndex = 0, totalWritersInQueue = 0, totalReadersInQueue = 0, nextWriterIndex = 0, nextReaderIndex = 0,
		numberOfReaders, numberOfWriters, totalProcesses, processId, readerCount = 0, isWriting = 0, data_transfer_tag = 1;

	int writerQueue[MAXPROC];
	int readerQueue[MAXPROC];
    
    MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &processId);
	MPI_Comm_size (MPI_COMM_WORLD, &totalProcesses);

	numberOfWriters = ((totalProcesses - 1) / 4);
	numberOfReaders = totalProcesses - numberOfWriters;

	if (processId == 0) { 
		// processo controlador, o processo pricipal que funciona como um servidor 
		// que atende as requisições dos leitores e escritores

		while (1) {
			char message[4];
			MPI_Recv(&message, 4, MPI_CHAR, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);
			int sourceProcess = (int) message[2] - 48; // pega o endereço do remetente da mensagem

			if (message[0] == 'R' && message[1] == 'R') { // mensagem de leitor requisitando leitura (RR = RequestRead)
				if (isWriting == 0) { // envia OK para o processo leitor, área crítica está apta para uso
					printf("\nRequisição de leitura aceita, processo de id %d irá ler.", sourceProcess); 
					MPI_Send(&sourceProcess, 1, MPI_INT, sourceProcess, data_transfer_tag, MPI_COMM_WORLD);
					readerCount++;
				}
				else { // adciona na fila, área crítica está ocupada com escrita
					printf("\nJá há um processo escrevendo e processo de id %d será adicionado a fila de leitores.", sourceProcess); 
					readerQueue[nextReaderIndex] = sourceProcess;
					totalReadersInQueue++;
					nextReaderIndex++;
				}

			}
			else if (message[0] == 'E' && message[1] == 'R') { // mensagem de leitor sinalizando que não está mais lendo (ER = EndRead)
				readerCount--;

				// não há mais processos efetuando a leitura, primeiro escritor da fila pode escrever, é enviado OK
				if (readerCount == 0 && totalWritersInQueue > 0) {
					printf("\nRequisição de escrita aceita, processo de id %d, que está no início da fila irá escrever.", writerQueue[writerStartIndex]); 
					MPI_Send(&writerQueue[writerStartIndex], 1, MPI_INT, writerQueue[writerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 1;
					// remove writer da fila
					writerQueue[writerStartIndex] = 0;
					writerStartIndex++;
					totalWritersInQueue--;

					if (totalWritersInQueue == 0) {
						printf("\nFila de escritores vazia."); 
						writerStartIndex = 0;
						nextWriterIndex = 0;
					}
				}
			}
			else if (message[0] == 'R' && message[1] == 'W') { // mensagem de escritor requisitando escrita (RW = RequestWriter)
				if (readerCount == 0 && isWriting == 0) {
					printf("\nRequisição de escrita aceita, processo de id %d irá escrever.", sourceProcess); 
					MPI_Send(&sourceProcess, 1, MPI_INT, sourceProcess, data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 0;
				}
				else {
					printf("\nJá há um ou mais processos lendo e processo de id %d será adicionado a fila de escritores.", sourceProcess); 
					writerQueue[nextWriterIndex] = sourceProcess;
					totalWritersInQueue++;
					nextWriterIndex++;
				}
			}
			else if (message[0] == 'E' && message[1] == 'W') {  // mensagem de escritor sinalizando que não está mais escrevendo (EW = EndWriter)
				isWriting = 0;

				if (totalReadersInQueue == 0 && totalWritersInQueue > 0) { // terminou de escrever mas ainda tem processo querendo escrever
					printf("\nTérmino de escrita, processo de id %d no início da fila de escritores irá escrever.", writerQueue[writerStartIndex]);  
					MPI_Send(&writerQueue[writerStartIndex], 1, MPI_INT, writerQueue[writerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 1;
					// remove writer da fila
					writerQueue[writerStartIndex] = 0;
					writerStartIndex++;
					totalWritersInQueue--;

					if (totalWritersInQueue == 0) {
						printf("\nFila de escritores vazia."); 
						writerStartIndex = 0;
						nextWriterIndex = 0;
					}
				}
				else { // não há mais processos querendo escrever, todos os processos de leitura serão atendidos
					while (totalReadersInQueue > 0) {
						printf("\nNão há mais processos escrevendo, processo de id %d no topo da fila de leitores irá ler.", readerQueue[readerStartIndex]);  
						MPI_Send(&readerQueue[readerStartIndex], 1, MPI_INT, readerQueue[readerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
						readerCount++;

						// remove reader da fila
						printf("\nProcesso de id %d removido da fila de leitores.", readerQueue[readerStartIndex]);
						readerQueue[readerStartIndex] = 0;
						readerStartIndex++;
						totalReadersInQueue--;

						if (totalReadersInQueue == 0) {
							printf("\nFila de leitores vazia."); 
							readerStartIndex = 0;
							nextReaderIndex = 0;
						}
					}
				}
			}
		    
		}
	}
	else if (processId <= numberOfWriters) { // processos escritores
		int requestMessage;
		char messageDocument[4];

   		while (1) {
   			sprintf(messageDocument, "RW%d", processId);

			sleep(1); 
			printf("\nEscritor %d quer escrever", processId);  
			MPI_Send(messageDocument, 4, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
			MPI_Recv(&requestMessage, 1, MPI_INT, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);

			// área crítica
			printf("\nEscritor %d está escrevendo", processId);  
			sleep(1);  
			printf("\nEscritor %d terminou de escrever", processId); 
			// área crítica completada 

			sprintf(messageDocument, "EW%d", processId);
			MPI_Send(messageDocument, 4, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
   		}

	}
	else { // processos leitores
		int requestMessage;
	    char messageDocument[4];

	    while (1) {
   			sprintf(messageDocument, "RR%d", processId);

			sleep(1); 
			printf("\nLeitor %d quer ler", processId);  
			MPI_Send(messageDocument, 4, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
			MPI_Recv(&requestMessage, 1, MPI_INT, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);
			
			// área crítica
			printf("\nLeitor %d está lendo", processId);  
			sleep(1) ;  
			printf("\nLeitor %d terminou de ler", processId);  
			// área crítica completada 
			
			sprintf(messageDocument, "ER%d", processId);
			MPI_Send(messageDocument, 4, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
		}

	}

	MPI_Finalize();
	
	printf("\nFim da execução");  
}