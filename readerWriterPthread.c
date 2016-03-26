/*
 * readerWriterMPI.c
 *
 * Trabalho de implementação da disciplina Sistemas Distribuídos
 *
 *  Created on: 2016
 *      Author: Leonardo Ramos, Carlos de Oliveira, Bruno Moraes
 *  
 *  Compilar:
 *      gcc -pthread readerWriterPthread.c -o readerWriterPthread
 */

#include <stddef.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "mpi.h"

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
  
void * reader(void * );  
void * writer (void * );  
sem_t wsem, mutex;  
int readcount = 0;  

main() { 
	int i = 0, numberOfReaderThread = 0, numberOfWriterThread;
	
	system("clear");  
	
	sem_init(&wsem, 0, 1);  
	sem_init(&mutex, 0, 1);  

	pthread_t readers_thr[500], writer_thr[500];
	
	printf("\nEntre com o número de leitores thread(MAX 500):\n");
	scanf("%d", &numberOfReaderThread);

	printf("\nEntre com o número de escritores thread(MAX 500):\n");
	scanf("%d", &numberOfWriterThread);

	for (i = 0; i < numberOfReaderThread; i++) {
		pthread_create(&readers_thr[i], NULL, reader, (void * ) i);
	}

	for (i = 0; i < numberOfWriterThread;i++) {
		pthread_create(&writer_thr[i], NULL, writer, (void *) i);
	}

	for (i = 0; i < numberOfWriterThread; i++) {
		pthread_join(writer_thr[i], NULL);
	}

	for (i = 0; i < numberOfReaderThread; i++) {
		pthread_join(readers_thr[i], NULL);
	}

	sem_destroy(&wsem);
	sem_destroy(&mutex);  
	
	printf("\nFim da execução");  
}

void * reader(void * arg) {
	while (1) {  
		int c = (int) arg;  

		sleep(1);
		printf("\nLeitor %d quer ler", c);    
		
		sem_wait(&mutex);  
		readcount++;  
		
		if (readcount == 1) {  
		   sem_wait(&wsem);
		}  
		sem_post(&mutex);  

		/*Critcal Section */  
		printf("\nLeitor %d está lendo", c);  
		sleep(1) ;  
		printf("\nLeitor %d terminou de ler", c);  
		/* critical section completd */  
		
		sem_wait(&mutex);  
		readcount--;  
		
		if (readcount == 0) { 
		    sem_post(&wsem);
		}  

		sem_post(&mutex);
	}  
}  

void * writer(void * arg) {  
	while (1) {
		int c = (int) arg ;  
		
		sleep(1);  
		printf("\nEscritor %d quer escrever", c);  
		
		sem_wait(&wsem);  
		
		printf("\nEscritor %d está escrevendo", c) ;  
		sleep(1);  
		printf("\nEscritor %d terminou de escrever", c);  
		
		sem_post(&wsem);
	}  
}