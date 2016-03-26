/*
 * readerWriterMPI.c
 *
 * Trabalho de implementação da disciplina Sistemas Distribuídos
 *
 *  Created on: 2016
 *      Author: Leonardo Ramos, Carlos de Oliveira, Bruno Moraes
 *  
 *  Compilar:
 *		mpicc readerWriterMPI.c -o readerWriterMPI
 *
 *  Executar:
 *		mpirun -np NUMERO_DE_PROCESSOS ./readerWriterMPI	
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
	int readerIndex = 0, writerIndex = 0, readerStartIndex = 0, writerStartIndex = 0,
		totalWritersInQueue = 0, totalReadersInQueue = 0, numberOfReaders, numberOfWriters, 
	    totalProcesses, processId, readerCount = 0, isWriting = 0, data_transfer_tag = 1;

	int writerQueue[MAXPROC];
	int readerQueue[MAXPROC];
    
    MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &processId);
	MPI_Comm_size (MPI_COMM_WORLD, &totalProcesses);

	numberOfWriters = ((totalProcesses - 1) / 2);
	numberOfReaders = totalProcesses - numberOfWriters;

	if (processId == 0) { 
		// processo controlador, o processo pricipal que funciona como um servidor 
		// que atende as requisições dos leitores e escritores

		while (1) {
			char message[50];
			MPI_Recv(&message, 20, MPI_CHAR, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);

			if (strcmp(message, "RequestRead") != 0) { // mensagem de leitor requisitando leitura
				if (isWriting == 0) { // envia OK para o processo leitor, área crítica está apta para uso
					MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, data_transfer_tag, MPI_COMM_WORLD);
					readerCount++;
				}
				else { // adciona na fila, área crítica está ocupada com escrita
					totalReadersInQueue++;
					readerQueue[readerIndex] = status.MPI_SOURCE;
					readerIndex++;
				}

			}
			else if (strcmp(message, "EndRead") != 0) { // mensagem de leitor sinalizando que não está mais lendo
				readerCount--;

				// não há mais processos efetuando a leitura, primeiro escritor da fila pode escrever, é enviado OK
				if (readerCount == 0 && totalWritersInQueue > 0) {
					MPI_Send(&writerQueue[writerStartIndex], 1, MPI_INT, writerQueue[writerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 1;
					// remove writer da fila
					writerStartIndex++;
					writerQueue[writerStartIndex] = 0;
					totalWritersInQueue--;

					if (totalWritersInQueue == 0) {
						writerStartIndex = 0;
						writerIndex = 0;
					}
				}
			}
			else if (strcmp(message, "RequestWrite") != 0) { // mensagem de escritor requisitando escrita
				if (readerCount == 0 && isWriting == 0) {
					MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, status.MPI_SOURCE, data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 0;
				}
				else {
					totalWritersInQueue++;
					writerQueue[writerIndex] = status.MPI_SOURCE;
					writerIndex++;
				}
			}
			else { // mensagem de escritor sinalizando que não está mais escrevendo
				isWriting = 0;

				if (totalReadersInQueue == 0 && totalWritersInQueue > 0) { // terminou de escrever mas ainda tem processo querendo escrever
					MPI_Send(&writerQueue[writerStartIndex], 1, MPI_INT, writerQueue[writerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
					isWriting = 1;
					// remove writer da fila
					writerStartIndex++;
					writerQueue[writerStartIndex] = 0;
					totalWritersInQueue--;

					if (totalWritersInQueue == 0) {
						writerStartIndex = 0;
						writerIndex = 0;
					}
				}
				else { // não há mais processos querendo escrever, todos os processos de leitura serão atendidos
					while (totalReadersInQueue > 0) {
						MPI_Send(&readerQueue[readerStartIndex], 1, MPI_INT, readerQueue[readerStartIndex], data_transfer_tag, MPI_COMM_WORLD);
						readerCount++;

						// remove reader da fila
						readerStartIndex++;
						readerQueue[readerStartIndex] = 0;
						totalReadersInQueue--;

						if (totalReadersInQueue == 0) {
							readerStartIndex = 0;
							readerIndex = 0;
						}
					}
				}
			}
		    
		}
	}
	else if (processId <= numberOfWriters) { // processos escritores
		int requestMessage;

   		while (1) {
			sleep(1); 
			printf("\nEscritor %d quer escrever", processId);  
			MPI_Send("RequestWrite", 20, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
			MPI_Recv(&requestMessage, 1, MPI_INT, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);

			// área crítica
			printf("\nEscritor %d está escrevendo", processId);  
			sleep(1);  
			printf("\nEscritor %d terminou de escrever", processId); 
			// área crítica completada 

			MPI_Send("EndWrite", 20, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
   		}

	}
	else { // processos leitores
		int requestMessage;
	    
	    while (1) {
			sleep(1); 
			printf("\nLeitor %d quer ler", processId);  
			MPI_Send("RequestRead", 20, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
			MPI_Recv(&requestMessage, 1, MPI_INT, MPI_ANY_SOURCE, data_transfer_tag, MPI_COMM_WORLD, &status);
			
			// área crítica
			printf("\nLeitor %d está lendo", processId);  
			sleep(1) ;  
			printf("\nLeitor %d terminou de ler", processId);  
			// área crítica completada 
			
			MPI_Send("EndRead", 20, MPI_CHAR, 0, data_transfer_tag, MPI_COMM_WORLD);
		}

	}

	MPI_Finalize();
	
	printf("\nFim da execução");  
}